
#include "Ravl2/Type/Type.hh"

#if 0
#include "Ravl/Type/Value.hh"
#include "Ravl/Type/Class.hh"
#include "Ravl/Type/Namespace.hh"
#include "Ravl/XMLFactory.hh"
#include "Ravl/Type/ClassMemberData.hh"
#include "Ravl/Index.hh"
#include "Ravl/Buffer.hh"
#include "Ravl/Exception.hh"
#include "Ravl/Hash.hh"
#include "Ravl/SysLog.hh"
#include "Ravl/Threads/RWLock.hh"
#endif

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 { namespace Types {

#if 0
  //! Destructor.
  TypeVisitorC::~TypeVisitorC()
  {}

  //! Visit value
  bool TypeVisitorC::Enter(const TypeC &theType,size_t n,ValueC &value)
  {
    ONDEBUG(RavlDebug("Enter '%s' %u",theType.Name().data(),(unsigned) n));
    return true;
  }

  void TypeVisitorC::Visit(const TypeC &theType,ValueC &value)
  {
    ONDEBUG(RavlDebug("Visit '%s' ",theType.Name().data()));
  }

  //! Const visit value
  void TypeVisitorC::Leave(const TypeC &theType,size_t n,ValueC &value)
  {
    ONDEBUG(RavlDebug("Leave '%s' %u",theType.Name().data(),(unsigned) n));
  }

  // ----------------------------------------------

  //! Destructor.
  TypeVisitorConstC::~TypeVisitorConstC()
  {}

  //! Const visit value
  bool TypeVisitorConstC::Enter(const TypeC &theType,size_t n,const ValueC &value)
  {
    ONDEBUG(RavlDebug("Enter '%s' %u",theType.Name().data(),(unsigned) n));
    return true;
  }

  void TypeVisitorConstC::Visit(const TypeC &theType,const ValueC &value)
  {
    ONDEBUG(RavlDebug("Visit '%s' ",theType.Name().data()));
  }

  //! Const visit value
  void TypeVisitorConstC::Leave(const TypeC &theType,size_t n,const ValueC &value)
  {
    ONDEBUG(RavlDebug("Leave '%s' %u",theType.Name().data(),(unsigned) n));
  }

  // --------------------------------------------------------------------

  class InstanceBufferC
    : public RCBodyVC
  {
  public:
    void *Data()
    { return reinterpret_cast<void *>(&(this[1])); }


    InstanceBufferC(const TypeC &theType)
     : m_theType(&theType)
    { m_theType->Construct(Data()); }
    //: Default constructor.

    ~InstanceBufferC()
    { m_theType->Destroy(Data()); }
    //: Destructor.

    static InstanceBufferC *Alloc(const TypeC &theType) {
      void *mem = malloc(sizeof(InstanceBufferC) + (theType.MemorySize()));
      if(mem == 0) {
        RavlError("Memory allocation failed, out of memory. ");
        throw ExceptionOutOfMemoryC("Out of memory. ");
      }
      InstanceBufferC *ret = reinterpret_cast<InstanceBufferC *> (mem);
      try {
        new(ret) InstanceBufferC(theType);
      } catch(...) {
        free(ret);
        throw ;
      }
      return ret;
    }

    typedef SmartPtrC<InstanceBufferC> RefT;
  protected:
    TypeC::RefT m_theType;
  };

  // --------------------------------------------------------------------


  static HashC<const char *,TypeC::RefT> &TypeMap() {
    static HashC<const char *,TypeC::RefT> table;
    return table;
  }

  const TypeC *TypeLookup(const std::type_info &typeInfo) {
    RWLockHoldC lock(TypeLock(),RavlN::RWLOCK_READONLY);
    TypeC::RefT *theResult = TypeMap().Lookup(typeInfo.name());
    if(theResult == 0)
      return 0;
    RavlAssert(theResult->IsValid());
    return theResult->BodyPtr();
  }

  const TypeC &TypeLookupAlways(const std::type_info &typeInfo) {
    const TypeC *ret = TypeLookup(typeInfo) ;
    if(ret == 0) {
      RavlError("Failed to lookup type info for '%s' ",RavlN::TypeName(typeInfo));
      throw RavlN::ExceptionOperationFailedC("Failed to lookup unknown type.");
    }
    return *ret;
  }


  //! Register type globally
  const TypeC *TypeC::RegisterType(const TypeC *theType) {
    RavlAssert(theType != 0);
    RWLockHoldC lock(TypeLock(),RavlN::RWLOCK_WRITE);
    const std::type_info *typeInfo = theType->TypeInfo();
    if(typeInfo != 0) {
      TypeC::RefT &theEntry = TypeMap()[typeInfo->name()];
      ONDEBUG(RavlDebug("Registering typename '%s' ",typeInfo->name()));
      if(theEntry.IsValid()) {
        RavlWarning("Typename '%s' already registered.",typeInfo->name());
        throw RavlN::ExceptionOperationFailedC("Duplicate type registration");
      }
      theEntry = theType;
    }
    NativeNamespace().Add(*theType);
    return theType;
  }

  //! Register type globally
  const ClassC *TypeC::RegisterType(ClassC *theType)
  {
    RavlAssert(theType != 0);
    if(!theType->IsConst())
      theType->Publish();
    RegisterType(static_cast<const TypeC *>(theType));
    return theType;
  }

  //! Register type globally
  const ClassC *TypeC::RegisterType(const ClassC *theType) {
    RavlAssert(theType != 0);
    RavlAssert(theType->IsConst());
    RegisterType(static_cast<const TypeC *>(theType));
    return theType;
  }


  //! Default constructor
  TypeC::TypeC()
    : m_role(TR_Value),
      m_precedence(0)
  {}

  //! Construct with just a name
  TypeC::TypeC(const StringC &name,TypeRoleT role,IntT precedence)
   : ScopeC(name),
     m_role(role),
     m_precedence(precedence)
  {
  }

  //! Construct from a factory.
  TypeC::TypeC(const XMLFactoryContextC &factory)
   : ScopeC(factory),
     m_role(TR_Value),
     m_precedence(0)
  {}

  //! Load from binary stream
  TypeC::TypeC(BinIStreamC &strm)
   : ScopeC(strm)
  {
    ByteT version = 0;
    strm >> version;
    if(version != 1)
      throw RavlN::ExceptionUnexpectedVersionInStreamC("ScopeC");
    UInt32T roleVal;
    UInt32T precedence;
    strm >> roleVal >> precedence;
    m_role = (TypeRoleT) roleVal;
    m_precedence = precedence;
  }

  //! Load from binary stream
  TypeC::TypeC(std::istream &strm)
  : m_role(TR_Value),
    m_precedence(0)
  {
    RavlAssertMsg(0,"not supported");
  }

  //! Save to binary stream
  bool TypeC::Save(BinOStreamC &strm) const
  {
    ScopeC::Save(strm);
    ByteT version = 1;
    strm << version;
    strm << (UInt32T) m_role << (Int32T) m_precedence;
    return true;
  }

  //! Save to text stream
  bool TypeC::Save(std::ostream &strm) const {
    ScopeC::Save(strm);
    return true;
  }

  //! Access native type info for class if it exists.
  const std::type_info *TypeC::TypeInfo() const
  { return 0; }

  //! Access amount of memory used to allocate object.
  size_t TypeC::MemorySize() const
  {
    RavlError("Memory size not implemented in '%s' ",RavlN::TypeName(typeid(*this)));
    RavlAssertMsg(0,"not implemented");
    return 0;
  }

  //! Test if class is abstract, if it is it can't be instantiated directly
  bool TypeC::IsAbstract() const
  { return false; }

  //! Actual type of value, this will resolve abstract values to the concrete class type
  const TypeC &TypeC::ActualType(const void *value) const
  { return *this; }

  //! Initialise variable in place
  void TypeC::CreateCopy(ValueC &handle,const void *data) const {
    ONDEBUG(RavlDebug("Creating copy of value. "));
    Create(handle);
    Assign(handle.Data(),data);
  }

  //! Create instance of type
  void TypeC::Create(ValueC &handle) const
  {
    InstanceBufferC *instBuffer = InstanceBufferC::Alloc(*this);
    handle.Set(instBuffer->Data(),*this,*instBuffer);
  }

  //! Initialise variable
  void TypeC::Construct(void *fieldPtr) const
  {
    RavlAssertMsg(0,"not implemented");
  }

  //! Destroy variable
  void TypeC::Destroy(void *fieldPtr) const
  {
    RavlAssertMsg(0,"not implemented");
  }

  //! Assign variable from the same type.
  void TypeC::Assign(void *dest,const void *src) const
  {
    RavlAssertMsg(0,"not implemented");
  }

  //! Visit all members of the type.
  void TypeC::Accept(TypeVisitorC &visitor,ValueC &value) const
  {
    visitor.Visit(*this,value);
  }

  //! Visit all members of the type.
  void TypeC::Accept(TypeVisitorConstC &visitor,const ValueC &value) const
  {
    visitor.Visit(*this,value);
  }


  //! Access the number of fields.
  size_t TypeC::NumFields() const
  { return 0; }

  //! Access field
  void TypeC::Field(size_t m,const ValueC &obj,ValueC &val) const
  {
    RavlAssertMsg(0,"invalid call");
  }

  //! Find a named field.
  bool TypeC::FindNamedField(const StringC &fieldName,size_t &n) const
  { return false; }

  //! Access type of field
  const ClassMemberDataC &TypeC::MemberData(size_t n) const
  {
    //ClassMemberDataC tmp;
    RavlError("Type %s does not have member data. ",RavlN::TypeName(typeid(*this)));
    RavlAssertMsg(0,"Type does not have member data.");
    //return TypeVoid();
    throw RavlN::ExceptionOperationFailedC("No such member");
  }

  //! Test if two values are equal.
  bool TypeC::Equals(const ValueC &v1,const ValueC &v2) const {
    RavlAssert(&v1.DataType() == this);
    return (v1.Data() == v2.Data());
  }


  class DumpVisitorC
   : public TypeVisitorConstC
  {
  public:
    DumpVisitorC(std::ostream &strm)
     : m_depth(0),
       m_strm(strm)
    {}

    StringC Indent() {
      StringC ret;
      for(int i = 0;i < m_depth;i++) {
        ret += "  ";
      }
      return ret;
    }


    //! Const visit value
    bool Enter(const TypeC &theType,size_t n,const ValueC &value) {
      m_depth++;
      m_strm << Indent() << theType.MemberData(n).Type().Name() << " " << theType.MemberData(n).Name() << "\n";
      return true;
    }

    //! Const visit value
    void Visit(const TypeC &theType,const ValueC &value) {
    }

    //! Const visit value
    void Leave(const TypeC &theType,size_t n,const ValueC &value) {
      m_depth--;
    }


    int m_depth;
    std::ostream &m_strm;
  };


  //! Test if this type is derived from another
  bool TypeC::IsDerivedFrom(const TypeC &type) const
  { return (&type == this); }

  //! Test if this type is the same as or derived from baseType
  bool TypeC::IsDerivedFrom(const ClassC &baseType) const {
    return IsDerivedFrom(static_cast<const TypeC &>(baseType));
  }

  //! Promote a handle to its most derived type.
  //! Returns true if val's type has been changed.
  bool TypeC::Promote(ValueC &val) const
  { return false; }

  //! Convert value to stream.
  bool TypeC::ToStream(std::ostream &strm,const void *value) const
  {
    strm << "(*unknown*)";
    return false;
  }

  //! Read value from text stream.
  //! May not be supported for all types
  bool TypeC::FromStream(void *value,std::istream &strm) const
  {
    return false;
  }

  //! Test if class is a native type
  bool TypeC::IsNative() const
  { return false; }

  //! Convert value to text stream.
  bool TypeC::ToBinStream(RavlN::BinOStreamC &strm,const void *value) const {
    RavlAssertMsg(0,"not implemented");
    return false;
  }

  //! Read value from text stream.
  //! May not be supported for all types
  bool TypeC::FromBinStream(void *value,RavlN::BinIStreamC &strm) const {
    RavlAssertMsg(0,"not implemented");
    return false;
  }

  //! Create value from binary stream.
  bool TypeC::CreateFromBinStream(RavlN::BinIStreamC &strm,ValueC &value) const {
    Create(value);
    return FromBinStream(value.Data(),strm);
  }

  //! Attempt to create type from a string.
  bool TypeC::CreateFromString(const StringC &txt,ValueC &value) const {
    Create(value);
    RavlN::StrIStreamC iss(txt);
    return FromStream(value.Data(),iss);
  }

  //! Compute a hash value for the type
  size_t TypeC::Hash(const ValueC & val) const {
    RavlWarning("Hash not implemented for type %s ",Name().c_str());
    return MemorySize();
  }


  //! Dump structure in a human readable form.
  void TypeC::Dump(std::ostream &out) const {
    ValueC tmp;
    DumpVisitorC dumper(out);
    Accept(dumper,tmp);
  }

  const TypeC &TypeNull() {
    static TypeC::RefT typeNull = new TypeC("");
    return *typeNull;
  }

  const TypeC &TypeVoid() {
    static TypeC::RefT typeVoid = new TypeC("void");
    return *typeVoid;
  }

  //! Write to binary stream.
  // This checks if the type is native, and write just the name name if it is.
  RavlN::BinOStreamC &operator<<(RavlN::BinOStreamC &strm,const TypeC::RefT &typeRef) {
    RavlN::ByteT saveMode = 1;
    if(typeRef.IsValid()) {
      if(typeRef->IsNative()) {
        saveMode = 2;
        //RavlDebug("Using save mode %d ",(int) saveMode);
        strm << saveMode << typeRef->Name();
      } else {
        saveMode = 3;
        //RavlDebug("Using save mode %d ",(int) saveMode);
        strm << saveMode;
        typeRef->Save(strm);
      }
    } else {
      //RavlDebug("Using save mode %d ",(int) saveMode);
      strm << saveMode;
    }
    return strm;
  }

  //! Read to binary stream.
  // This checks if the type is native, and read just the name name if it is.
  RavlN::BinIStreamC &operator>>(RavlN::BinIStreamC &strm,TypeC::RefT &typeRef) {
    RavlN::ByteT saveMode = 0;
    strm >> saveMode;
    switch(saveMode) {
      case 1: // Nill ptr.
        typeRef.Invalidate();
        break;
      case 2: { // Native named type.
        StringC theTypeName;
        strm >> theTypeName;
        if(!NativeNamespace().Lookup(theTypeName,typeRef)) {
          RavlError("Type '%s' not known. ",theTypeName.c_str());
          throw RavlN::ExceptionOperationFailedC("Failed to load type.");
        }
      } break;
      case 3:
        RavlN::LoadStreamImpl(strm,typeRef,*typeRef);
        break;
      default:
        RavlError("Unexpected save mode %u ",(unsigned) saveMode);
        throw RavlN::ExceptionInvalidStreamC("Invalid stream");
    }
    return strm;
  }

  bool operator!=(const TypeC &t1,const TypeC &t2)
  { return &t1 != &t2; }

  bool operator==(const TypeC &t1,const TypeC &t2)
  { return &t1 == &t2; }

  static RavlN::TypeNameC g_type(typeid(TypeC::RefT),"RavlN::SmartPtrC<RavlN::TypeN::TypeC>");
#endif

  static std::mutex &TypeLock() {
    static std::mutex access;
    return access;
  }

  //! Access native namespace.
  Namespace &NativeNamespace() {
    static Namespace::Ref nativeNamespace = new Namespace("::");
    return *nativeNamespace;
  }

}}
