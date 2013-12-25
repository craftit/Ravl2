#ifndef RAVL2_TYPE_TYPE_HH
#define RAVL2_TYPE_TYPE_HH 1

#include "Ravl2/Type/Namespace.hh"

#include <string>
#include <vector>

namespace Ravl2 { namespace Types {

#if 0
  class Variant;
  class Type;
  class Class;
  class ClassMemberData;
  class Namespace;
  class Binds;

  //! Visitor base class.
  class TypeVisitor {
  public:
    //! Destructor.
    virtual ~TypeVisitor();

    //! Visit value
    //! Enter field number 'n'.
    //! If false is returned the field will not be processed further.
    virtual bool Enter(const Type &theType,size_t n,Variant &value);

    //! Visit value
    virtual void Visit(const Type &theType,Variant &value);

    //! Visit value
    //! Leave field number 'n'.
    virtual void Leave(const Type &theType,size_t n,Variant &value);
  };

  //! Visitor base class to a const object.

  class TypeVisitorConst {
  public:
    //! Destructor.
    virtual ~TypeVisitorConst();

    //! Const visit value
    //! Enter field number 'n'.
    //! If false is returned the field will not be processed further.
    virtual bool Enter(const TypeC &theType,size_t n,const Variant &value);

    //! Const visit value
    virtual void Visit(const TypeC &theType,const Variant &value);

    //! Const visit value
    //! Leave field number 'n'.
    virtual void Leave(const TypeC &theType,size_t n,const Variant &value);
  };
#endif

  enum class TypeRole
  {
    value,
    constraint,
    mixed
  };

  //! Type description

  class Type
    : public Scope
  {
  public:
    //! Default constructor
    Type();

    //! Construct with just a name
    Type(const std::string &name,TypeRole role = TypeRole::value,int precedence = 0);

    //! Construct from a factory.
    //Type(const Factory &factory);

    //! Access native type info for class if it exists.
    virtual const std::type_info *cpptypeinfo() const;
#if 0
    //! Access amount of memory used to allocate object.
    virtual size_t MemorySize() const;

    //! Test if class is abstract, if it is it can't be instantiated directly.
    virtual bool IsAbstract() const;

    //! Actual type of value, this will resolve handles to parent class type to the actual concrete class type instantiated.
    virtual const TypeC &ActualType(const void *value) const;

    //! Test if class is a native type.
    // This is used to decide if we should emit type information with binary files.
    virtual bool IsNative() const;

    //! Create instance of type
    virtual void Create(Variant &handle) const;

    //! Initialise variable in place
    virtual void CreateCopy(Variant &handle,const void *data) const;

    //! Initialise variable in place
    //! 'fieldPtr' must point to a piece of memory at least MemorySize() bytes long.
    virtual void Construct(void *fieldPtr) const;

    //! Destroy variable
    virtual void Destroy(void *fieldPtr) const;

    //! Assign variable from the same type.
    virtual void Assign(void *dest,const void *src) const;

    //! Visit all members of the type.
    virtual void Accept(TypeVisitorC &visitor,Variant &value) const;

    //! Visit all members of the type.
    virtual void Accept(TypeVisitorConstC &visitor,const Variant &value) const;

    //! Convert value to text stream.
    virtual bool ToStream(std::ostream &strm,const void *value) const;

    //! Read value from text stream.
    //! May not be supported for all types
    virtual bool FromStream(void *value,std::istream &strm) const;

    //! Convert value to binary stream.
    virtual bool ToBinStream(RavlN::BinOStreamC &strm,const void *value) const;

    //! Read value from binary stream.
    //! May not be supported for all types
    virtual bool FromBinStream(void *value,RavlN::BinIStreamC &strm) const;

    //! Create value from binary stream.
    virtual bool CreateFromBinStream(RavlN::BinIStreamC &strm,Variant &value) const;

    //! Attempt to create type from a string.
    virtual bool CreateFromString(const StringC &string,Variant &value) const;

    //! Access the number of fields.
    virtual size_t NumFields() const;

    //! Access field in 'obj' assign it to 'val.
    virtual void Field(size_t m,const Variant &obj,Variant &val) const;

    //! Find a named field.
    virtual bool FindNamedField(const StringC &fieldName,size_t &n) const;

    //! Access type of field
    virtual const ClassMemberDataC &MemberData(size_t n) const;

    //! Test if two values are equal.
    virtual bool Equals(const Variant &value1,const Variant &value2) const;

    //! Test if this type is the same as or derived from baseType
    virtual bool IsDerivedFrom(const TypeC &baseType) const;

    //! Test if this type is the same as or derived from baseType
    bool IsDerivedFrom(const ClassC &baseType) const;

    //! Promote a handle to its most derived type.
    //! Returns true if val's type has been changed.
    virtual bool Promote(Variant &val) const;

    //! Compute a hash value for the type
    virtual size_t Hash(const Variant &val) const;

    //! Get role for type when used for matching.
    TypeRoleT Role() const
    { return m_role; }

    //! Set the role information
    void SetRole(TypeRoleT role)
    { m_role = role; }

    //! Precedence of type
    int Precedence() const
    { return m_precedence; }

    //! Dump structure in a human readable form.
    void Dump(std::ostream &out) const;

    //! Register type globally
    static const TypeC *RegisterType(const TypeC *theType);

    //! Register type globally
    static const ClassC *RegisterType(ClassC *theType);

    //! Register type globally
    static const ClassC *RegisterType(const ClassC *theType);

    //! Handle to type.
    typedef RavlN::SmartPtr<Type> Ref;
  protected:
    TypeRoleT m_role;
    int m_precedence;

    template<typename DataT>
    void MakeDefault(Variant &newValue,void *) const;

    template<typename DataT>
    void MakeDefault(Variant &newValue,RavlN::RCBodyC *) const;

    template<typename DataT>
    void MakeDefault(Variant &newValue,RavlN::RCBodyVC *) const;

    template<typename DataT>
    void MakeCopy(Variant &newValue,void *,const void *original) const;

    template<typename DataT>
    void MakeCopy(Variant &newValue,RavlN::RCBodyC *,const void *original) const;

    template<typename DataT>
    void MakeCopy(Variant &newValue,RavlN::RCBodyVC *,const void *original) const;
#endif
  };

#if 0
  //! Test if two types are different.
  bool operator!=(const TypeC &t1,const TypeC &t2);

  //! Test if two types are the same.
  bool operator==(const TypeC &t1,const TypeC &t2);

  //! Write to binary stream.
  // This checks if the type is native, and write just the name name if it is.
  RavlN::BinOStreamC &operator<<(RavlN::BinOStreamC &strm,const TypeC::RefT &typeRef);

  //! Read to binary stream.
  // This checks if the type is native, and read just the name name if it is.
  RavlN::BinIStreamC &operator>>(RavlN::BinIStreamC &strm,TypeC::RefT &typeRef);

  const TypeC &TypeNull();
  const TypeC &TypeVoid();
  const ClassC &TypeRCBody();
  const ClassC &TypeRCBodyV();

  const TypeC &TypeBool();
  const TypeC &TypeChar();
  const TypeC &TypeUnsignedChar();
  const TypeC &TypeInt();
  const TypeC &TypeUnsigned();
  const TypeC &TypeFloat();
  const TypeC &TypeDouble();
  const TypeC &TypeInt64();
  const TypeC &TypeUInt64();
  const TypeC &TypeInt16();
  const TypeC &TypeUInt16();
  const TypeC &TypeIndex();
  const TypeC &TypeStdString();
  const TypeC &TypeRavlString();
  const ClassC &TypeRavlVector2d();
  const ClassC &TypeRavlVector3d();
  const TypeC &TypeRavlVector();
  const TypeC &TypeRavlTVectorFloat();
  const TypeC &TypeDate();
  const TypeC &TypeUUId();
  const TypeC &TypeSmartPtr_Type();
  const TypeC &TypeSArray1d_Value();
  const TypeC &TypeSArray1d_bool();
  const TypeC &TypeSArray1d_double();
  const TypeC &TypeSArray1d_float();
  const TypeC &TypeSArray1d_char();
  const TypeC &TypeSArray1d_String();

  const TypeC &TypeOf(const bool &val);
  const TypeC &TypeOf(const int &val);
  const TypeC &TypeOf(const unsigned &val);
  const TypeC &TypeOf(const float &val);
  const TypeC &TypeOf(const double &val);
  const TypeC &TypeOf(const char &val);
  const TypeC &TypeOf(const unsigned char &val);
  const TypeC &TypeOf(const Int16T &val);
  const TypeC &TypeOf(const UInt16T &val);
  const TypeC &TypeOf(const Int64T &val);
  const TypeC &TypeOf(const long long &val);
  const TypeC &TypeOf(const UInt64T &val);
  const TypeC &TypeOf(const std::string &val);
  const TypeC &TypeOf(const RavlN::RCBodyC &val);
  const TypeC &TypeOf(const RavlN::RCBodyVC &val);
  const TypeC &TypeOf(const RavlN::IndexC &val);
  const TypeC &TypeOf(const RavlN::Index2dC &val);
  const TypeC &TypeOf(const RavlN::UUIdC &val);
  const TypeC &TypeOf(const RavlN::DateC &val);
  const TypeC &TypeOf(const RavlN::StringC &val);
  const TypeC &TypeOf(const RavlN::VectorC &val);
  const TypeC &TypeOf(const RavlN::TVectorC<float> &val);
  const TypeC &TypeOf(const RavlN::Vector2dC &val);
  const TypeC &TypeOf(const RavlN::Vector3dC &val);
  const TypeC &TypeOf(const RavlN::MeanVarianceC &val);
  const TypeC &TypeOf(const RavlN::SizeC &val);
  const TypeC &TypeOf(const TypeC &val);
  const TypeC &TypeOf(const Variant &val);
  const TypeC &TypeOf(const std::vector<StringC> &val);
  const TypeC &TypeOf(const std::vector<Variant> &val);
  const TypeC &TypeOf(const std::vector<int> &val);
  const TypeC &TypeOf(const std::vector<float> &val);
  const TypeC &TypeOf(const RavlN::SmartPtrC<TypeC> &val);
  const TypeC &TypeOf(const RavlN::SArray1dC<StringC> &val);
  const TypeC &TypeOf(const RavlN::SArray1dC<Variant> &val);
  const TypeC &TypeOf(const RavlN::SArray1dC<RavlN::SArray1dC<Variant> > &val);
  const TypeC &TypeOf(const RavlN::SArray1dC<float> &val);
  const TypeC &TypeOf(const RavlN::SArray1dC<double> &val);
  const TypeC &TypeOf(const RavlN::SArray1dC<char> &val);
  const TypeC &TypeOf(const RavlN::SArray1dC<bool> &val);
  const TypeC &TypeOf(const RavlN::SArray1dC<int> &val);
  const TypeC &TypeOf(const RavlN::SArray1dC<unsigned char> &val);
  const TypeC &TypeOf(const RavlN::CollectionC<StringC> &val);
  const TypeC &TypeOf(const RavlN::CollectionC<Variant> &val);
  const TypeC &TypeOf(const RavlN::CollectionC<float> &val);
  const TypeC &TypeOf(const RavlN::CollectionC<double> &val);

  const TypeC *TypeLookup(const std::type_info &typeInfo);
  const TypeC &TypeLookupAlways(const std::type_info &typeInfo);


  template<typename DataT>
  const TypeC &TypeOf(const DataT &val)
  {
    const TypeC *theType = TypeLookup(typeid(DataT));
    if(theType == 0) {
      RavlError("Type %s unknown.",RavlN::TypeName(typeid(DataT)));
      RavlAssertMsg(theType != 0,"Type unknown");
      throw RavlN::ExceptionOperationFailedC("Type unknown.");
    }
    return *theType;
  }
#endif

  //! Access native name space.
  Namespace &NativeNamespace();
}}


#endif
