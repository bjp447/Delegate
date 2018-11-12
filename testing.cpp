#include "Delegate.h"

#include "Actor.h"
#include "testclass.h"

#include "ObjectController.h"

struct functor {
	functor() {};

	int operator()(int a, int b)
	{
		return 1;
	}
};

SINGLE_CAST_DELEGATE(testEvent, int, int);
SINGLE_CAST_DELEGATE_RetVal(int, Te, int);
MULTI_CAST_DELEGATE(testEvent2, int, int);

SINGLE_CAST_DELEGATE(TT);
MULTI_CAST_DELEGATE(TEST);

void Test2()
{
	int c = 2;
}

void Test(int a, int b)
{
	int c = (a + b);
	//return &c;
}

int main()
{
	//-------------------------------------------------

	std::shared_ptr<Object> obj1(new Object());
	std::shared_ptr<testclass> test(new testclass());

	TEST erfer;
	erfer.AddBind([](){});
	erfer.AddBind(&Test2);
	erfer();

	testEvent2 testMulti;
	
	//TODO: Add functor support
	//functor func;
	//testMulti.AddBind(func);


	testMulti.AddBind(Test);
	
	testMulti.AddBind(obj1, &Object::test_param);
	testMulti.AddBind(obj1, &Object::test_param);
	testMulti.AddBind(obj1, &Object::test_param);
	testMulti.RemoveBindSingle(obj1, &Object::test_param);
	testMulti.RemoveBindAllInstance(obj1);

	testMulti.AddBindUnique(test, &testclass::test_param);
	testMulti.AddBindUnique(test, &testclass::test_param);
	testMulti.AddBind(test, &testclass::test_param);
	testMulti.AddBind(test, &testclass::test_param);
	testMulti.RemoveBind(test, &testclass::test_param);

	testMulti.AddBind(test, &testclass::test_param);
	testMulti.ContainsBind(test, &testclass::test_param);

	testMulti.AddBind(obj1, &Object::test_param);
	testMulti.ContainsBind(obj1, &Object::test_param2);

	int a = 2;
	int b = 4;

	testMulti.AddBind(obj1, &Object::test_param);
	testMulti.AddBind(obj1, &Object::test_param2);
	testMulti.AddBind(test, &testclass::test_param);

	std::cout << '\n' << '\n';
	testMulti.Broadcast(2, 4);
	std::cout << '\n' << '\n';
	testMulti.Broadcast(a, b);
	testMulti.Clear();

	//-------------------------------------------

	Object* ptrObj = new Object();
	testclass* tclass = new testclass();
	
	testEvent2 testMultiRaw;
	testMultiRaw.AddBind(ptrObj, &Object::test_param);
	testMultiRaw.AddBind(ptrObj, &Object::test_param);
	testMultiRaw.AddBind(ptrObj, &Object::test_param);
	testMultiRaw.RemoveBindSingle(ptrObj, &Object::test_param);
	testMultiRaw.RemoveBindAllInstance(ptrObj);

	testMultiRaw.AddBindUnique(tclass, &testclass::test_param);
	testMultiRaw.AddBindUnique(tclass, &testclass::test_param);
	testMultiRaw.AddBind(tclass, &testclass::test_param);
	testMultiRaw.AddBind(tclass, &testclass::test_param);
	testMultiRaw.RemoveBind(tclass, &testclass::test_param);
	testMultiRaw.ContainsBind(tclass, &testclass::test_param);

	testMultiRaw.Broadcast(2, 4);
	testMultiRaw.Broadcast(a, b);
	testMultiRaw.Clear();
	return 0;
}