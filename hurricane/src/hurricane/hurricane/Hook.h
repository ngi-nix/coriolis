// ****************************************************************************************************
// File: Hook.h
// Authors: R. Escassut
// Copyright (c) BULL S.A. 2000-2004, All Rights Reserved
// ****************************************************************************************************

#ifndef HURRICANE_HOOK
#define HURRICANE_HOOK

#include "hurricane/Hooks.h"

namespace Hurricane {

class Component;



// ****************************************************************************************************
// Hook declaration
// ****************************************************************************************************

class Hook {
// *********

// Attributes
// **********

    private: Hook* _nextHook;

// Constructors
// ************

    protected: Hook();

    private: Hook(const Hook& hook); // not implemented to forbid copy construction

// Destructor
// **********

    protected: virtual ~Hook();

// Operators
// *********

    private: Hook& operator=(const Hook& hook); // not implemented to forbid assignment

// Accessors
// *********

    public: virtual Component* getComponent() const = 0;

    public: Hook* getNextHook() const;
    public: Hook* getPreviousHook() const;

    public: Hook* getMasterHook() const;
    public: Hook* getNextMasterHook() const;
    public: Hook* getPreviousMasterHook() const;

    public: Hooks getHooks() const;
    public: Hooks getSlaveHooks() const;

// Filters
// *******

    public: static HookFilter getIsMasterFilter();

// Predicates
// **********

    public: virtual bool isMaster() const = 0;

    public: bool isAttached() const;

// Updators
// ********

    public: Hook* detach();
    public: Hook* attach(Hook* hook);
    public: Hook* merge(Hook* hook);
    public: void _setNextHook(Hook* hook);

// Others
// ******

    public: virtual string _getString() const = 0;
    public: virtual Record* _getRecord() const;

};



} // End of Hurricane namespace.


INSPECTOR_P_SUPPORT(Hurricane::Hook);


#endif // HURRICANE_HOOK

// ****************************************************************************************************
// Copyright (c) BULL S.A. 2000-2004, All Rights Reserved
// ****************************************************************************************************
