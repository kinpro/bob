/**
 * @file src/python/ip/src/Gabor.cc
 * @author <a href="mailto:Laurent.El-Shafey@idiap.ch">Laurent El Shafey</a> 
 * @date Thu 17 Mar 19:12:40 2011 
 *
 * @brief Binds the Gabor filters to python
 */

#include <boost/python.hpp>

#include "sp/convolution.h"
#include "ip/GaborSpatial.h"
#include "ip/GaborFrequency.h"
#include "ip/GaborBankSpatial.h"
#include "ip/GaborBankFrequency.h"

using namespace boost::python;
namespace ip = Torch::ip;

static const char* gabor_spatial_doc = "Objects of this class, after configuration, can filter images with a Gabor kernel, performing the operation in the spatial domain.";
static const char* gabor_bank_spatial_doc = "Objects of this class, after configuration, can filter images with a bank of Gabor kernels, performing the operation in the spatial domain.";
static const char* gabor_frequency_doc = "Objects of this class, after configuration, can filter images with a Gabor kernel, performing the operation in the frequency domain.";
static const char* gabor_bank_frequency_doc = "Objects of this class, after configuration, can filter images with a bank of Gabor kernels, performing the operation in the frequency domain.";


void bind_ip_gabor() {
  enum_<Torch::ip::Gabor::NormOption>("GaborNorm")
    .value("NoNorm", Torch::ip::Gabor::NoNorm)
    .value("SpatialFactor", Torch::ip::Gabor::SpatialFactor)
    .value("ZeroMeanUnitVar", Torch::ip::Gabor::ZeroMeanUnitVar)
    ;

  class_<ip::GaborSpatial, boost::shared_ptr<ip::GaborSpatial> >("GaborSpatial", gabor_spatial_doc, init<optional<const double, const double, const double, const double, const int, const bool, const enum Torch::ip::Gabor::NormOption, const enum Torch::sp::Convolution::BorderOption> >((arg("f")="0.25", arg("theta")="0.", arg("gamma")="1.", arg("eta")="1.", arg("spatial_size")="35", arg("cancel_dc")="false",arg("norm")="SpatialFactor",arg("border_opt")="Mirror"), "Constructs a new Gabor filter in the spatial domain."))
    //.add_property("test", &ip::TanTriggs::getTest, &ip::TanTriggs::setTest)
    .def("__call__", (void (ip::GaborSpatial::*)(const blitz::Array<std::complex<double>,2>&, blitz::Array<std::complex<double>,2>&))&ip::GaborSpatial::operator(), (arg("input"), arg("output")), "Call an object of this type to filter an image.")
    ;

  class_<ip::GaborFrequency, boost::shared_ptr<ip::GaborFrequency> >("GaborFrequency", gabor_frequency_doc, init<const int, const int, optional<const double, const double, const double, const double, const int, const bool, /*const enum Torch::ip::Gabor::NormOption,*/ const enum Torch::sp::Convolution::BorderOption, const bool> >((arg("height"),arg("width"),arg("f")="0.25", arg("theta")="0.", arg("gamma")="1.", arg("eta")="1.", arg("spatial_size")="35", arg("cancel_dc")="false", /*arg("norm")="SpatialFactor",*/ arg("border_opt")="Mirror", arg("output_in_frequency")="false"), "Constructs a new Gabor filter in the spatial domain."))
    //.add_property("test", &ip::TanTriggs::getTest, &ip::TanTriggs::setTest)
    .def("__call__", (void (ip::GaborFrequency::*)(const blitz::Array<std::complex<double>,2>&, blitz::Array<std::complex<double>,2>&))&ip::GaborFrequency::operator(), (arg("input"), arg("output")), "Call an object of this type to filter an image.")
    ;

  class_<ip::GaborBankSpatial, boost::shared_ptr<ip::GaborBankSpatial> >("GaborBankSpatial", gabor_bank_spatial_doc, init<optional<const int, const int, const double, const bool, const double, const double, const double, const double, const int, const bool, const enum Torch::ip::Gabor::NormOption, const enum Torch::sp::Convolution::BorderOption> >((arg("n_orient"), arg("n_freq"), arg("fmax")="0.25", arg("orientation_full")="false", arg("k"), arg("p"), arg("gamma")="1.", arg("eta")="1.", arg("spatial_size")="35", arg("cancel_dc")="false", arg("norm")="SpatialFactor", arg("border_opt")="Mirror"), "Constructs a new Gabor filter bank in the spatial domain."))
    //.add_property("test", &ip::TanTriggs::getTest, &ip::TanTriggs::setTest)
    .def("__call__", (void (ip::GaborBankSpatial::*)(const blitz::Array<std::complex<double>,2>&, blitz::Array<std::complex<double>,2>&))&ip::GaborBankSpatial::operator(), (arg("input"), arg("output")), "Call an object of this type to filter an image.")
    ;

  class_<ip::GaborBankFrequency, boost::shared_ptr<ip::GaborBankFrequency> >("GaborBankFrequency", gabor_bank_frequency_doc, init<const int, const int, optional<const int, const int, const double, const bool, const double, const double, const double, const double, const int, const bool, /*const enum Torch::ip::Gabor::NormOption,*/ const enum Torch::sp::Convolution::BorderOption, const bool> >((arg("height"), arg("width"), arg("n_orient"), arg("n_freq"), arg("fmax")="0.25", arg("orientation_full")="false", arg("k"), arg("p"), arg("gamma")="1.", arg("eta")="1.", arg("spatial_size")="35", arg("cancel_dc")="false", /*arg("norm")="SpatialFactor",*/ arg("border_opt")="Mirror", arg("output_in_frequency")="false"), "Constructs a new Gabor filter in the spatial domain."))
    //.add_property("test", &ip::TanTriggs::getTest, &ip::TanTriggs::setTest)
    .def("__call__", (void (ip::GaborFrequency::*)(const blitz::Array<std::complex<double>,2>&, blitz::Array<std::complex<double>,2>&))&ip::GaborFrequency::operator(), (arg("input"), arg("output")), "Call an object of this type to filter an image.")
    ;


}