// -*- C++ -*-
//
// Copyright (c) BULL S.A. 2000-2015, All Rights Reserved
//
// This file is part of Hurricane.
//
// Hurricane is free software: you can redistribute it  and/or  modify
// it under the terms of the GNU  Lesser  General  Public  License  as
// published by the Free Software Foundation, either version 3 of  the
// License, or (at your option) any later version.
//
// Hurricane is distributed in the hope that it will  be  useful,  but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHAN-
// TABILITY or FITNESS FOR A PARTICULAR PURPOSE. See  the  Lesser  GNU
// General Public License for more details.
//
// You should have received a copy of the Lesser  GNU  General  Public
// License along with Hurricane. If not, see
//                                     <http://www.gnu.org/licenses/>.
//
// +-----------------------------------------------------------------+
// |                  H U R R I C A N E                              |
// |     V L S I   B a c k e n d   D a t a - B a s e                 |
// |                                                                 |
// |  Author      :                       Remy Escassut              |
// |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Header  :  "./hurricane/Slot.h"                            |
// +-----------------------------------------------------------------+


#ifndef  HURRICANE_SLOT_H
#define  HURRICANE_SLOT_H

#ifndef  HURRICANE_COMMONS_H
#error "Slot.h musn't be included alone, please uses Commons.h."
#endif


namespace Hurricane {

// -------------------------------------------------------------------
// Class  :  "Slot".

  class Slot {

    public:
    // Destructor.
      virtual              ~Slot          ();
    // Accessors.
      static  size_t        getAllocateds ();
      const   string&       getName       () const;
      virtual string        getDataString () const = 0;
      virtual Record*       getDataRecord () const = 0;
      virtual Slot*         getClone      () const = 0;

    protected:
    // Internal: Static Attributes.
      static  size_t        _allocateds;
    // Internal: Attributes.
              const string  _name;

    protected:
    // Internal: Constructors & Destructors.
                            Slot          ( const string& name );
  };


// Inline Member Functions.
  inline  Slot::Slot    ( const string& name ) : _name(name)
  {
    _allocateds++;
  //cerr << "Slot::Slot() - " << _name << " " << hex << (void*)this << endl;
  }

  inline const string& Slot::getName () const { return _name; }


// -------------------------------------------------------------------
// Class  :  "SlotTemplate<Data>".

  template<typename Data>
  class SlotTemplate : public Slot {

    public:
    // Constructor.
                             SlotTemplate  ( const string& name, Data data );
                             SlotTemplate  (       string& name, Data data );
    // Accessors.
      virtual string         getDataString () const;
      virtual Record*        getDataRecord () const;
      virtual SlotTemplate<Data>*
                             getClone      () const;

    protected:
    // Internal: Attributes.
              Data           _data;

    private:
    // Internal: Constructors.
                             SlotTemplate  ( const SlotTemplate& );
              SlotTemplate&  operator=     ( const SlotTemplate& );
  };


// Inline Member Functions.
  template<typename Data>
  SlotTemplate<Data>::SlotTemplate ( const string& name, Data data )
                                   : Slot(name), _data(data) { }

  template<typename Data>
  SlotTemplate<Data>::SlotTemplate ( string& name, Data data )
                                   : Slot(name), _data(data) { }

  template<typename Data>
  string  SlotTemplate<Data>::getDataString () const { return ::getString(_data); }

  template<typename Data>
  Record* SlotTemplate<Data>::getDataRecord () const { return ::getRecord(_data); }

  template<typename Data>
  SlotTemplate<Data>* SlotTemplate<Data>::getClone () const
  { return new SlotTemplate(_name,_data); }


// -------------------------------------------------------------------
// Class  :  "SlotTemplate<const Data>".

  template<typename Data>
  class SlotTemplate<const Data> : public Slot {

    public:
    // Constructor.
                             SlotTemplate  ( const string& name, const Data data );
                             SlotTemplate  (       string& name, const Data data );
    // Accessors.
      virtual string         getDataString () const;
      virtual Record*        getDataRecord () const;
      virtual SlotTemplate<const Data>*
                             getClone      () const;

    protected:
    // Internal: Attributes.
              const Data     _data;

    private:
    // Internal: Constructors.
                             SlotTemplate  ( const SlotTemplate& );
              SlotTemplate&  operator=     ( const SlotTemplate& );
  };


// Inline Member Functions.
  template<typename Data>
  SlotTemplate<const Data>::SlotTemplate ( const string& name, const Data data )
                                         : Slot(name), _data(data) { }

  template<typename Data>
  SlotTemplate<const Data>::SlotTemplate ( string& name, const Data data )
                                         : Slot(name), _data(data) { }

  template<typename Data>
  string  SlotTemplate<const Data>::getDataString () const { return getString(_data); }

  template<typename Data>
  Record* SlotTemplate<const Data>::getDataRecord () const { return getRecord(_data); }

  template<typename Data>
  SlotTemplate<const Data>* SlotTemplate<const Data>::getClone () const
  { return new SlotTemplate(_name,_data); }


// -------------------------------------------------------------------
// Class  :  "SlotTemplate".


  template<typename Data>
  class SlotTemplate<Data*> : public Slot {

    public:
    // Constructor.
                           SlotTemplate  ( const string& name, Data* data );
                           SlotTemplate  (       string& name, Data* data );
    // Accessors.
      virtual string       getDataString () const;
      virtual Record*      getDataRecord () const;
      virtual SlotTemplate<Data*>*
                           getClone      () const;

    protected:
    // Internal: Attributes.
              Data*        _data;

    private:
    // Internal: Constructors.
                           SlotTemplate  ( const SlotTemplate& );
              SlotTemplate& operator=    ( const SlotTemplate& );
  };


// Inline Member Functions.
  template<typename Data>
  SlotTemplate<Data*>::SlotTemplate ( const string& name, Data* data )
                                    : Slot(name), _data(data) {}

  template<typename Data>
  SlotTemplate<Data*>::SlotTemplate ( string& name, Data* data )
                                    : Slot(name), _data(data) {}

  template<typename Data>
  string  SlotTemplate<Data*>::getDataString () const { return ::getString(_data); }

  template<typename Data>
  Record* SlotTemplate<Data*>::getDataRecord () const { return ::getRecord(_data); }

  template<typename Data>
  SlotTemplate<Data*>* SlotTemplate<Data*>::getClone () const
  { return new SlotTemplate(_name,_data); }


// -------------------------------------------------------------------
// Class  :  "SlotTemplate".


  template<typename Data>
  class SlotTemplate<Data* const> : public Slot {

    public:
    // Constructor.
                           SlotTemplate  ( const string& name, Data* const data );
                           SlotTemplate  (       string& name, Data* const data );
    // Accessors.
      virtual string       getDataString () const;
      virtual Record*      getDataRecord () const;
      virtual SlotTemplate<Data* const>*
                           getClone      () const;

    protected:
    // Internal: Attributes.
              Data* const  _data;

    private:
    // Internal: Constructors.
                           SlotTemplate  ( const SlotTemplate& );
              SlotTemplate& operator=    ( const SlotTemplate& );
  };


// Inline Member Functions.
  template<typename Data>
  SlotTemplate<Data* const>::SlotTemplate ( const string& name, Data* const data )
                                          : Slot(name), _data(data) {}

  template<typename Data>
  SlotTemplate<Data* const>::SlotTemplate ( string& name, Data* const data )
                                          : Slot(name), _data(data) {}

  template<typename Data>
  string  SlotTemplate<Data* const>::getDataString () const { return ::getString(_data); }

  template<typename Data>
  Record* SlotTemplate<Data* const>::getDataRecord () const { return ::getRecord(_data); }

  template<typename Data>
  SlotTemplate<Data* const>* SlotTemplate<Data* const>::getClone () const
  { return new SlotTemplate(_name,_data); }


// -------------------------------------------------------------------
// Class  :  "SlotTemplate".


  template<>
  class SlotTemplate<Record*> : public Slot {

    public:
    // Constructor.
      inline                      SlotTemplate  ( const string& name, Record* data );
      inline                      SlotTemplate  (       string& name, Record* data );
    // Accessors.
      inline virtual string       getDataString () const;
      inline virtual Record*      getDataRecord () const;
      inline virtual SlotTemplate<Record*>*
                                  getClone      () const;

    protected:
    // Internal: Attributes.
                     Record*      _data;

    private:
    // Internal: Constructors.
                           SlotTemplate  ( const SlotTemplate& );
              SlotTemplate& operator=    ( const SlotTemplate& );
  };


// Inline Member Functions.
  inline SlotTemplate<Record*>::SlotTemplate ( const string& name, Record* data )
                                             : Slot(name), _data(data) {}

  inline SlotTemplate<Record*>::SlotTemplate ( string& name, Record* data )
                                             : Slot(name), _data(data) {}

  inline string  SlotTemplate<Record*>::getDataString () const { return _name; }
  inline Record* SlotTemplate<Record*>::getDataRecord () const { return _data; }

  inline SlotTemplate<Record*>* SlotTemplate<Record*>::getClone () const
  { return new SlotTemplate<Record*>(_name,_data); }


} // Hurricane namespace.


template<typename Data>
inline Hurricane::Slot* getSlot( std::string& name, Data d )
{
//std::cerr << "getSlot<string&,Data>( \"" << name << "\" )" << std::endl;
  return new Hurricane::SlotTemplate<Data> ( name, d );
}


template<typename Data>
inline Hurricane::Slot* getSlot( std::string& name, Data* d )
{
//std::cerr << "getSlot<string&,Data*>( \"" << name << "\" )" << std::endl;
  return new Hurricane::SlotTemplate<Data*> ( name, d );
}


template<typename Data>
inline Hurricane::Slot* getSlot( const std::string& name, Data d )
{
//std::cerr << "getSlot<const string&,Data>( \"" << name << "\" )" << std::endl;
  return new Hurricane::SlotTemplate<Data> ( name, d );
}


template<typename Data>
inline Hurricane::Slot* getSlot( const std::string& name, Data* d )
{
//std::cerr << "getSlot<const string&,Data*>( \"" << name << "\" )" << std::endl;
  return new Hurricane::SlotTemplate<Data*> ( name, d );
}

#endif // HURRICANE_SLOT_H
