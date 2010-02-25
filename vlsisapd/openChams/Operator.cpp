/*
 *  Operator.cpp
 *  openChams
 *
 *  Created by damien dupuis on 08/02/10.
 *  Copyright 2010 UPMC / LIP6. All rights reserved.
 *
 */

#include <string>
using namespace std;

#include "Operator.h"
#include "OpenChamsException.h"

namespace OpenChams {
Operator::Operator(Name operatorName, Name simulModel, unsigned callOrder) 
    : _name(operatorName)
    , _simulModel(simulModel)
    , _callOrder(callOrder) {}
    
void Operator::addConstraint(Name paramName, Name ref, Name refParam, double factor) {
    map<Name, Constraint*>::iterator it = _constraints.find(paramName);
    if (it != _constraints.end()) {
        string error("[ERROR] Cannot set two constraints for the same parameter (");
        error += paramName.getString();
        error += ").";
        throw OpenChamsException(error);
    }
    _constraints[paramName] = new Operator::Constraint(ref, refParam, factor);
}
    
void Operator::addConstraint(Name paramName, Name refEquation, double factor) {
    map<Name, Constraint*>::iterator it = _constraints.find(paramName);
	if (it != _constraints.end()) {
        string error("[ERROR] Cannot set two constraints for the same parameter (");
        error += paramName.getString();
        error += ").";
        throw OpenChamsException(error);
    }
    _constraints[paramName] = new Operator::Constraint(Name(""), refEquation, factor);
}
    
Operator::Constraint::Constraint(Name ref, Name refParam, double factor)
    : _ref(ref)
    , _refParam(refParam)
    , _factor(factor) {}
} // namespace
