A dynamic delegate system for C++.
Supports single and multicast delegates. 
Single cast delegates support return types, multiple arguemnets, and any arguemnet type.
Multicast delegates support multiple arguemnets, and any arguemnet type.

The delegates allow for smart pointers, raw pointers, member functions, regular functions, or stateless lambdas.
The delegates allocate memory on the heap.

Users can instantiate delegates by using one of the three macro defines: 
    SINGLE_CAST_DELEGATE('variable name', arg types...);
    SINGLE_CAST_DELEGATE_RetVal(return type, 'variable name', arg types...);
    MULTI_CAST_DELEGATE('variable name', arg types...);
    
    
Future Updates:
  SingleCast payload delegates.
  Lambdas with states.
