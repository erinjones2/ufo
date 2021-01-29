/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef UFO_FILTERS_VARIABLEASSIGNMENT_H_
#define UFO_FILTERS_VARIABLEASSIGNMENT_H_

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "ioda/core/ObsData.h"
#include "ioda/core/ParameterTraitsObsDtype.h"
#include "oops/base/ObsFilterParametersBase.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"
#include "ufo/filters/ObsProcessorBase.h"
#include "ufo/utils/parameters/ParameterTraitsVariable.h"

namespace ufo {

/// Parameters controlling assignment of new values to a variable.
class AssignmentParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(AssignmentParameters, Parameters)

 public:
  /// Name of the variable to which new values should be assigned.
  oops::RequiredParameter<std::string> name{"name", this};

  /// Set of channels to which new values should be assigned.
  oops::Parameter<std::string> channels{"channels", "", this};

  /// Value to be assigned to the specified variable (at all locations selected be the `where`
  /// statement, if present).
  ///
  /// Exactly one of the `value` and `function` options must be given.
  oops::OptionalParameter<std::string> value_{"value", this};

  /// Variable (typically an ObsFunction) that should be evaluated and assigned to the specified
  /// variable (at all locations selected be the `where` statement, if present).
  ///
  /// Exactly one of the `value` and `function` options must be given.
  oops::OptionalParameter<ufo::Variable> function{"function", this};

  /// Type (int, float, string or datetime) of the variable to which new values should be assigned.
  ///
  /// This option must be provided if the variable doesn't exist yet. If this option is provided
  /// and the variable already exists, its type must match the value of this option,
  /// otherwise an exception will be thrown.
  oops::OptionalParameter<ioda::ObsDtype> type{"type", this};

  /// This function is overridden to check that mutually exclusive options aren't specified
  /// together, throwing an exception otherwise.
  void deserialize(util::CompositePath &path, const eckit::Configuration &config) override;
};

/// Parameters controlling the VariableAssignment filter.
class VariableAssignmentParameters : public oops::ObsFilterParametersBase {
  OOPS_CONCRETE_PARAMETERS(VariableAssignmentParameters, ObsFilterParametersBase)

 public:
  /// One or more sets of options controlling the values assigned to a particular variable.
  oops::Parameter<std::vector<AssignmentParameters>> assignments{"assignments", {}, this};

  /// Conditions used to select locations where variable assignment should be performed.
  /// If not specified, variable assignment will be performed at all locations.
  oops::Parameter<eckit::LocalConfiguration> where{"where", eckit::LocalConfiguration(), this};

  /// If set to true, variable assignment will be done after the obs operator has been invoked
  /// (even if the filter doesn't require any variables from the GeoVaLs or HofX groups).
  oops::Parameter<bool> deferToPost{"defer to post", false, this};
};

/// \brief Assigns specified values to elements of specified variables selected by the where
/// statement.
///
/// The values can be constants or vectors generated by ObsFunctions. If the variables don't exist
/// yet, they will be created; in this case elements not selected by the where clause will be
/// initialized with the missing value markers.
///
/// Example 1: Create new variables `air_temperature@GrossErrorProbability` and
/// `relative_humidity@GrossErrorProbability` and set them to 0.1 at all locations.
///
///     filter: Variable Assignment
///     assignments:
///     - name: air_temperature@GrossErrorProbability
///       type: float  # type must be specified if the variable doesn't already exist
///       value: 0.1
///     - name: relative_humidity@GrossErrorProbability
///       type: float
///       value: 0.1
///
/// Example 2: Set `air_temperature@GrossErrorProbability` to 0.05 at all locations in the tropics.
///
///     filter: Variable Assignment
///     where:
///     - variable:
///         name: latitude@MetaData
///       minvalue: -30
///       maxvalue:  30
///     assignments:
///     - name: air_temperature@GrossErrorProbability
///       value: 0.05
///
/// Example 3: Set `relative_humidity@GrossErrorProbability` to values computed by an ObsFunction
/// (0.1 in the southern extratropics and 0.05 in the northern extratropics, with a linear
/// transition in between).
///
///     filter: Variable Assignment
///     assignments:
///     - name: relative_humidity@GrossErrorProbability
///       function:
///         name: ObsErrorModelRamp@ObsFunction
///         options:
///           xvar:
///             name: latitude@MetaData
///           x0: [-30]
///           x1: [30]
///           err0: [0.1]
///           err1: [0.05]
///
class VariableAssignment : public ObsProcessorBase,
                           private util::ObjectCounter<VariableAssignment> {
 public:
  /// The type of parameters accepted by the constructor of this filter.
  /// This typedef is used by the FilterFactory.
  typedef VariableAssignmentParameters Parameters_;

  static const std::string classname() {return "ufo::VariableAssignment";}

  VariableAssignment(ioda::ObsSpace & obsdb, const Parameters_ & parameters,
                     std::shared_ptr<ioda::ObsDataVector<int> > flags,
                     std::shared_ptr<ioda::ObsDataVector<float> > obserr);

 private:
  void print(std::ostream &) const override;
  void doFilter() const override;

  Parameters_ parameters_;
};

}  // namespace ufo

#endif  // UFO_FILTERS_VARIABLEASSIGNMENT_H_
