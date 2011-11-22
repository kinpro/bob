/**
 * @author <a href="mailto:andre.dos.anjos@gmail.com">Andre Anjos</a> 
 * @date Mon 21 Feb 13:54:28 2011 
 *
 * @brief Implementation of MatUtils (handling of matlab .mat files)
 */

#include <boost/format.hpp>

#include "io/MatUtils.h"
#include "io/reorder.h"

namespace io = Torch::io;
namespace iod = io::detail;
namespace ca = Torch::core::array;

boost::shared_ptr<mat_t> iod::make_matfile(const std::string& filename,
    int flags) {
  return boost::shared_ptr<mat_t>(Mat_Open(filename.c_str(), flags), std::ptr_fun(Mat_Close));
}

/**
 * This method will create a new boost::shared_ptr to matvar_t that knows how
 * to delete itself
 *
 * You pass it the file and the variable name to be read out or a combination
 * of parameters required to build a new matvar_t from scratch (see matio API
 * for details).
 */
static boost::shared_ptr<matvar_t> make_matvar(boost::shared_ptr<mat_t>& file) {

  return boost::shared_ptr<matvar_t>(Mat_VarReadNext(file.get()), std::ptr_fun(Mat_VarFree));

}

/**
 * This is essentially like make_matvar(), but uses VarReadNextInfo() instead
 * of VarReadNext(), so it does not load the data, but it is faster.
 */
static boost::shared_ptr<matvar_t> 
make_matvar_info(boost::shared_ptr<mat_t>& file) {

  return boost::shared_ptr<matvar_t>(Mat_VarReadNextInfo(file.get()), std::ptr_fun(Mat_VarFree));

}

static boost::shared_ptr<matvar_t> make_matvar(boost::shared_ptr<mat_t>& file,
   const std::string& varname) {

  if (!varname.size()) throw io::Uninitialized();
  return boost::shared_ptr<matvar_t>(Mat_VarRead(file.get(), const_cast<char*>(varname.c_str())), std::ptr_fun(Mat_VarFree));

}

/**
 * Returns the MAT_C_* enumeration for the given ElementType
 */
static enum matio_classes mio_class_type (ca::ElementType i) {
  switch (i) {
    case ca::t_int8: 
      return MAT_C_INT8;
    case ca::t_int16: 
      return MAT_C_INT16;
    case ca::t_int32: 
      return MAT_C_INT32;
    case ca::t_int64: 
      return MAT_C_INT64;
    case ca::t_uint8: 
      return MAT_C_UINT8;
    case ca::t_uint16: 
      return MAT_C_UINT16;
    case ca::t_uint32: 
      return MAT_C_UINT32;
    case ca::t_uint64: 
      return MAT_C_UINT64;
    case ca::t_float32:
      return MAT_C_SINGLE;
    case ca::t_complex64:
      return MAT_C_SINGLE;
    case ca::t_float64:
      return MAT_C_DOUBLE;
    case ca::t_complex128:
      return MAT_C_DOUBLE;
    default:
      {
        boost::format f("data type '%s' is not supported by matio backend");
        f % ca::stringize(i);
        throw std::invalid_argument(f.str().c_str());
      }
  }
}

/**
 * Returns the MAT_T_* enumeration for the given ElementType
 */
static enum matio_types mio_data_type (ca::ElementType i) {
  switch (i) {
    case ca::t_int8: 
      return MAT_T_INT8;
    case ca::t_int16: 
      return MAT_T_INT16;
    case ca::t_int32: 
      return MAT_T_INT32;
    case ca::t_int64: 
      return MAT_T_INT64;
    case ca::t_uint8: 
      return MAT_T_UINT8;
    case ca::t_uint16: 
      return MAT_T_UINT16;
    case ca::t_uint32: 
      return MAT_T_UINT32;
    case ca::t_uint64: 
      return MAT_T_UINT64;
    case ca::t_float32:
      return MAT_T_SINGLE;
    case ca::t_complex64:
      return MAT_T_SINGLE;
    case ca::t_float64:
      return MAT_T_DOUBLE;
    case ca::t_complex128:
      return MAT_T_DOUBLE;
    default:
      {
        boost::format f("data type '%s' is not supported by matio backend");
        f % ca::stringize(i);
        throw std::invalid_argument(f.str().c_str());
      }
  }
}

/**
 * Returns the ElementType given the matio MAT_T_* enum and a flag indicating
 * if the array is complex or not (also returned by matio at matvar_t)
 */
static ca::ElementType torch_element_type (int mio_type, bool is_complex) {

  ca::ElementType eltype = ca::t_unknown;

  switch(mio_type) {

    case(MAT_T_INT8): 
      eltype = ca::t_int8;
      break;
    case(MAT_T_INT16): 
      eltype = ca::t_int16;
      break;
    case(MAT_T_INT32):
      eltype = ca::t_int32;
      break;
    case(MAT_T_INT64):
      eltype = ca::t_int64;
      break;
    case(MAT_T_UINT8):
      eltype = ca::t_uint8;
      break;
    case(MAT_T_UINT16):
      eltype = ca::t_uint16;
      break;
    case(MAT_T_UINT32):
      eltype = ca::t_uint32;
      break;
    case(MAT_T_UINT64):
      eltype = ca::t_uint64;
      break;
    case(MAT_T_SINGLE):
      eltype = ca::t_float32;
      break;
    case(MAT_T_DOUBLE):
      eltype = ca::t_float64;
      break;
    default:
      return ca::t_unknown;
  }

  //if type is complex, it is signalled slightly different
  if (is_complex) {
    if (eltype == ca::t_float32) return ca::t_complex64;
    else if (eltype == ca::t_float64) return ca::t_complex128;
    else return ca::t_unknown;
  }
  
  return eltype;
}

boost::shared_ptr<matvar_t> make_matvar
(const std::string& varname, const ca::interface& buf) {

  const ca::typeinfo& info = buf.type();
  void* fdata = static_cast<void*>(new char[info.buffer_size()]);
  
  //matio gets dimensions as integers
  int mio_dims[TORCH_MAX_DIM];
  for (size_t i=0; i<info.nd; ++i) mio_dims[i] = info.shape[i];

  switch (info.dtype) {
    case ca::t_complex64:
    case ca::t_complex128:
    case ca::t_complex256:
      {
        //special treatment for complex arrays
        uint8_t* real = static_cast<uint8_t*>(fdata);
        uint8_t* imag = real + (info.buffer_size()/2);
        io::row_to_col_order_complex(buf.ptr(), real, imag, info); 
        ComplexSplit mio_complex = {real, imag};
        return boost::shared_ptr<matvar_t>(Mat_VarCreate(varname.c_str(),
              mio_class_type(info.dtype), mio_data_type(info.dtype),
              info.nd, mio_dims, static_cast<void*>(&mio_complex),
              MAT_F_COMPLEX),
            std::ptr_fun(Mat_VarFree));
      }
    default:
      break;
  }

  io::row_to_col_order(buf.ptr(), fdata, info); ///< data copying!

  return boost::shared_ptr<matvar_t>(Mat_VarCreate(varname.c_str(),
        mio_class_type(info.dtype), mio_data_type(info.dtype),
        info.nd, mio_dims, fdata, 0), std::ptr_fun(Mat_VarFree));
}

/**
 * Assigns a single matvar variable to an ca::interface. Re-allocates the buffer
 * if required.
 */
static void assign_array (boost::shared_ptr<matvar_t> matvar, ca::interface& buf) {

  ca::typeinfo info(torch_element_type(matvar->data_type, matvar->isComplex),
      matvar->rank, matvar->dims);

  if(!buf.type().is_compatible(info)) buf.set(info);

  if (matvar->isComplex) {
    ComplexSplit mio_complex = *static_cast<ComplexSplit*>(matvar->data);
    io::col_to_row_order_complex(mio_complex.Re, mio_complex.Im, buf.ptr(), info);
  }
  else io::col_to_row_order(matvar->data, buf.ptr(), info);

}

void iod::read_array (boost::shared_ptr<mat_t> file, ca::interface& buf,
    const std::string& varname) {

  boost::shared_ptr<matvar_t> matvar;
  if (varname.size()) matvar = make_matvar(file, varname);
  else matvar = make_matvar(file);
  if (!matvar) throw Torch::io::Uninitialized();
  assign_array(matvar, buf);

}

void iod::write_array(boost::shared_ptr<mat_t> file, 
    const std::string& varname, const ca::interface& buf) {

  boost::shared_ptr<matvar_t> matvar = make_matvar(varname, buf);
  Mat_VarWrite(file.get(), matvar.get(), 0);

}

/**
 * Given a matvar_t object, returns our equivalent ca::typeinfo struct.
 */
static void get_var_info(boost::shared_ptr<const matvar_t> matvar,
    ca::typeinfo& info) {
  info.set(torch_element_type(matvar->data_type, matvar->isComplex),
      matvar->rank, matvar->dims);
}

void iod::mat_peek(const std::string& filename, ca::typeinfo& info) {

  boost::shared_ptr<mat_t> mat = iod::make_matfile(filename, MAT_ACC_RDONLY);
  if (!mat) throw io::FileNotReadable(filename);
  boost::shared_ptr<matvar_t> matvar = make_matvar(mat); //gets the first var.
  get_var_info(matvar, info);

}

void iod::mat_peek_set(const std::string& filename, ca::typeinfo& info) {
  boost::shared_ptr<mat_t> mat = iod::make_matfile(filename, MAT_ACC_RDONLY);
  if (!mat) throw io::FileNotReadable(filename);
  boost::shared_ptr<matvar_t> matvar = make_matvar(mat); //gets the first var.
  get_var_info(matvar, info);
}

boost::shared_ptr<std::map<size_t, std::pair<std::string, ca::typeinfo> > >
  iod::list_variables(const std::string& filename) {

  boost::shared_ptr<std::map<size_t, std::pair<std::string, ca::typeinfo> > > retval(new std::map<size_t, std::pair<std::string, ca::typeinfo> >());

  boost::shared_ptr<mat_t> mat = iod::make_matfile(filename, MAT_ACC_RDONLY);
  if (!mat) throw io::FileNotReadable(filename);
  boost::shared_ptr<matvar_t> matvar = make_matvar(mat); //gets the first var.

  size_t id = 0;
 
  //now that we have found a variable, fill the array
  //properties taking that variable as basis
  (*retval)[id] = std::make_pair(matvar->name, ca::typeinfo());
  get_var_info(matvar, (*retval)[id].second);
  const ca::typeinfo& type_cache = (*retval)[id].second;

  if ((*retval)[id].second.dtype == ca::t_unknown) {
    throw io::TypeError((*retval)[id].second.dtype, ca::t_float32);
  }

  //if we got here, just continue counting the variables inside. we
  //only read their info since that is faster -- but attention! if we just read
  //the varinfo, we don't get typing correct, so we copy that from the previous
  //read variable and hope for the best.

  while ((matvar = make_matvar_info(mat))) {
    (*retval)[++id] = std::make_pair(matvar->name, type_cache);
  }

  return retval;
}
