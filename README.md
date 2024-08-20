# RAVL2

Recognition And Vision Library 2

Goals:

 - Efficient interoperability with other vision libraries such as opencv
 - Port algorithms from RAVL to use new c++ idioms


 - Some routines are adapted from ccmath. the original code can be found at: https://github.com/adis300/ccmath


## Conventions

### Naming

 * Scoped objects like classes and namespaces are named with CamelCase. 
 * Functions and variables start with lower case.
 * Types such as template arguments end with a 'T'

### Argument order

The output is the first argument(s).

### Functions

 * fill(x,value) - fill x with value
 * copy(x,y) - copy y to x
 * clip(x,y) - Return an array x clipped to the range specified by y
 * clamp(x,y) - Return a value x limited to the range specified by y
 * view(x) - Return a view of x which can be used as a Ravl2 type, this view maybe reshaped without copying the data.
 * access(x) - Return a reference to the data in x.  Assigning to an access object has broadcast semantics.
 * clone(x) - Return a deep copy of x that maybe modified without affecting the original.

### Memory management

  * Array<x> - Contain a shared pointer to the data, copying an array does not copy the data.

