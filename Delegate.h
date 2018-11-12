/*
* Created by Brennan Pohl on 10/12/18.
* Copyright © 2018 Brennan Pohl. All rights reserved.
*
*
*
*/

#pragma once
#ifndef _DELEGATE_
#define _DELEGATE_

#include <memory>
#include <functional>
#include <type_traits>
#include <vector>

#define INDEX_NONE -1

#pragma region DelegateWrapers
//* Use for a context based delegate name.
//* First argument is the disired delegate name, followed by any amount of parameters.
//* Format: (DelegateName, type (optional), ...)
#define SINGLE_CAST_DELEGATE(DelegateName, ... ) \
	using DelegateName = DLG::DynamicSingleCastDelegate<void, __VA_ARGS__ >;

//* Use for a context based delegate name.
//* First argument is the disired delegate name, followed by any amount of parameters.
//* Format: (return type, DelegateName, type (optional), ...)
#define SINGLE_CAST_DELEGATE_RetVal(RetType, DelegateName, ... ) \
	using DelegateName = DLG::DynamicSingleCastDelegate<RetType, __VA_ARGS__ >;

//* Use for a context based delegate name.
//* First argument is the disired delegate name, followed by any amount of parameters.
//* Format: (DelegateName, type (optional), ...)
#define MULTI_CAST_DELEGATE(DelegateName, ... ) \
	using DelegateName = DLG::DynamicMultiCastDelegate<__VA_ARGS__ >;

#pragma endregion

namespace DLG
{

#pragma region DelegateBases
	template <typename RetType, typename... Params>
	class DelegateBase
	{
	public:
		virtual ~DelegateBase() {};

		inline virtual bool IsValid() const
		{
			return true;
		}

		inline virtual RetType Execute(Params... in) const
		{
			return static_cast<RetType>(0);
		}

		inline virtual void* const GetObjectPointer() const
		{
			return nullptr;
		}

	protected:
		DelegateBase() {};
	};

	template <typename Func, typename RetType = void, typename... Params>
	class FreeDelegate final : public DelegateBase<RetType, Params...>
	{
	private:
		Func Function;

	public:
		FreeDelegate(Func& func) :Function(func) {}
		virtual ~FreeDelegate() {}

		inline virtual RetType Execute(Params... in) const override final
		{
			return (*Function)(std::forward<Params>(in)...);
		}

		inline Func GetFunctionPtr() const
		{
			return this->Function;
		}
	};

	template <typename RetType, typename UserClass, typename Func, typename... Params>
	class MemberDelegate final : public DelegateBase<RetType, Params...>
	{
	public:
		using ObjectPointer = std::weak_ptr<UserClass>;

	private:
		ObjectPointer Object;
		Func Function;

	public:
		MemberDelegate(ObjectPointer obj, Func& func) :Object(obj), Function(func) {}
		virtual ~MemberDelegate() {}

		inline virtual bool IsValid() const override final
		{
			return !this->Object.expired();
		}

		inline virtual RetType Execute(Params... in) const override final
		{
			return ((*Object.lock()).*Function)(std::forward<Params>(in)...);
		}

		inline virtual void* const GetObjectPointer() const override final
		{
			return this->Object.lock().get();
		}

		inline Func GetFunctionPtr() const
		{
			return this->Function;
		}
	};

	template <typename RetType, typename UserClass, typename Func, typename... Params>
	class RawDelegate final : public DelegateBase<RetType, Params...>
	{
	public:
		using ObjectPointer = UserClass* const;

	private:
		ObjectPointer Object;
		Func Function;

	public:
		RawDelegate(ObjectPointer obj, Func& func) :Object(obj), Function(func) {}
		virtual ~RawDelegate() {}

		inline virtual RetType Execute(Params... in) const override final
		{
			return ((*Object).*Function)(std::forward<Params>(in)...);
		}

		inline virtual void* const GetObjectPointer() const override final
		{
			return this->Object;
		}

		inline Func GetFunctionPtr() const
		{
			return this->Function;
		}
	};

#pragma endregion


#pragma region SingleCast
	//* Format: <'return type' = void, 'arguement type' (optional), ...>
	template <typename RetType = void, typename... Params>
	class DynamicSingleCastDelegate
	{
	private:
		using FreeFunc = RetType(*)(Params...);
		using LambdaFunc_NoState = FreeFunc;
		using LambdaFunc_State = std::function< RetType(Params...)>;

	private:
		std::unique_ptr<DelegateBase<RetType, Params...>> member_ptr;

	public:
		DynamicSingleCastDelegate() {};
		virtual ~DynamicSingleCastDelegate()
		{
			this->Clear();
		}

		//*Binds method.
		template <class UserClass>
		void Bind(std::shared_ptr<UserClass> target, RetType(UserClass::*func)(Params...))
		{
			using Func = RetType(UserClass::*)(Params...);
			using MemberDelegateSig = MemberDelegate<RetType, UserClass, Func, Params...>;

			std::weak_ptr<UserClass> inst = target;
			this->member_ptr = std::unique_ptr<MemberDelegateSig>(new MemberDelegateSig(inst, func));
		}

		//*Binds method.
		template <class UserClass>
		void Bind(std::shared_ptr<UserClass> target, RetType(UserClass::*func)(Params...) const)
		{
			using Func = RetType(UserClass::*)(Params...);
			using ConstMemberDelegateSig = MemberDelegate<RetType, UserClass, Func const, Params...>;

			std::weak_ptr<UserClass> inst = target;
			this->member_ptr = std::unique_ptr<ConstMemberDelegateSig>(new ConstMemberDelegateSig > (inst, func));
		}

		//*Binds function.
		void Bind(FreeFunc func)
		{
			using FreeDelegateSig = FreeDelegate<FreeFunc, RetType, Params...>;

			this->UnBind();
			this->member_ptr = std::unique_ptr<FreeDelegateSig>(new FreeDelegateSig(func));
		}

		//*Binds method.
		template <class UserClass>
		void Bind(UserClass* const target, RetType(UserClass::*func)(Params...))
		{
			using Func = RetType(UserClass::*)(Params...);
			using ConstMemberDelegateSig = RawDelegate<RetType, UserClass, Func, Params...>;

			this->member_ptr = std::unique_ptr<ConstMemberDelegateSig>(new ConstMemberDelegateSig > (target, func));
		}

		//*Binds method.
		template <class UserClass>
		void Bind(UserClass* const target, RetType(UserClass::*func)(Params...) const)
		{
			using Func = RetType(UserClass::*)(Params...);
			using ConstMemberDelegateSig = RawDelegate<RetType, UserClass, Func const, Params...>;

			this->member_ptr = std::unique_ptr<ConstMemberDelegateSig>(new ConstMemberDelegateSig > (target, func));
		}

		//*UnBinds the methods attached to this delegate.
		inline void UnBind()
		{
			this->member_ptr.reset();
		}

		//*UnBinds the methods attached to this delegate.
		inline void Clear()
		{
			this->UnBind();
		}

		//* Calls binded functions.
		RetType Execute(Params... in)
		{
			if (this->member_ptr != nullptr && this->member_ptr->IsValid())
			{
				return this->member_ptr->Execute(std::forward<Params>(in)...);
			}
			else
			{
				this->UnBind();
				//throw std::runtime_error("No Function");
				std::cout << "ERROR: DynamicSingleCastDelegate::Execute() did not call bind. Return value may be unsafe.";
				return static_cast<RetType>(0);
			}
		}

		//* Calls binded functions.
		RetType Execute(Params... in) const
		{
			if (this->member_ptr != nullptr && this->member_ptr->IsValid())
			{
				return this->member_ptr->Execute(std::forward<Params>(in)...);
			}
			else
			{
				//throw std::runtime_error("No Function");
				std::cout << "ERROR: DynamicSingleCastDelegate::Execute() did not call bind. Return value may be unsafe.";
				return static_cast<RetType>(0);
			}
		}

		//@Return: True if object and method is bound; False, if not.
		inline bool IsBound() const
		{
			return this->member_ptr.get();
		}

		//*Execute
		inline void operator()(Params... in)
		{
			this->Execute(std::forward<Params>(in)...);
		}

	};
#pragma endregion


#pragma region MultiCast
	//* Format: <'arguement type' (optional), ...>
	template <typename... Params>
	class DynamicMultiCastDelegate
	{
	private:
		using FreeFunc = void(*)(Params...);
		using LambdaFunc_NoState = FreeFunc;
		using LambdaFunc_State = std::function< void(Params...)>;

		template<typename UserClass> struct MFSig
		{
			using MemberFunctionSignature = void (UserClass::*)(Params...);
			using MemberFunctionConstSignature = void (UserClass::*)(Params...) const;
		};

	private:
		std::vector<DelegateBase<void, Params...>*> Member_Binds;

	public:
		DynamicMultiCastDelegate() {};

		~DynamicMultiCastDelegate()
		{
			Clear();
		}

	private:
		void RemoveAt(int index)
		{
			if (index >= 0)
			{
				auto& back = this->Member_Binds.back();
				std::swap(this->Member_Binds.at(index), back);
				delete back;
				this->Member_Binds.pop_back();
			}
		}

		void Remove(DelegateBase<void, Params...>*& bind)
		{
			auto& back = this->Member_Binds.back();
			std::swap(bind, back);
			delete back;
			this->Member_Binds.pop_back();
		}

		template <typename UserClass>
		void _RemoveBindAllInstance(UserClass* const target)
		{
			for (unsigned i = 0; i < this->Member_Binds.size(); )
			{
				auto& bind = Member_Binds[i];

				if (target == bind->GetObjectPointer())
				{
					Remove(bind);
				}
				else
				{
					++i;
				}
			}
		}

		int FindBind(FreeFunc func, unsigned startingIndex = 0) const
		{
			using FreeFunctionSig = FreeDelegate<FreeFunc, void, Params...>;

			for (unsigned i = startingIndex; i < this->Member_Binds.size(); ++i)
			{
				auto bind = dynamic_cast<FreeFunctionSig*>(Member_Binds[i]);

				if (bind != nullptr && bind->GetFunctionPtr() == func)
				{
					return i;
				}
			}
			return INDEX_NONE;
		}

		template <typename DelSig, typename UserClass, typename Func>
		int FindBind(UserClass* const target, Func func, unsigned startingIndex = 0) const
		{
			for (unsigned i = startingIndex; i < this->Member_Binds.size(); ++i)
			{
				auto& Bind = Member_Binds[i];

				if (target == Bind->GetObjectPointer())
				{
					auto bind = static_cast<DelSig*>(Bind);

					if (bind != nullptr && bind->GetFunctionPtr() == func)
					{
						return i;
					}
				}
			}
			return INDEX_NONE;
		}

		template <typename Func, typename UserClass>
		void _BindUnique(std::shared_ptr<UserClass>& target, Func func)
		{
			using MemberDelegateSig = MemberDelegate<void, UserClass, Func, Params...>;

			if (_Contains<MemberDelegateSig>(target.get(), func) == false)
			{
				this->Member_Binds.push_back(new MemberDelegateSig(target, func));
			}
		}

		template <typename UserClass, typename Func>
		void _BindUnique(UserClass* const target, Func func)
		{
			using RawDelegateSig = RawDelegate<void, UserClass, Func, Params...>;

			if (_Contains<RawDelegateSig>(target, func) == false)
			{
				this->Member_Binds.push_back(new RawDelegateSig(target, func));
			}
		}

		template <typename UserClass, typename Func>
		void _Bind(std::shared_ptr<UserClass>& target, Func func)
		{
			using MemberDelegateSig = MemberDelegate<void, UserClass, Func, Params...>;
			this->Member_Binds.push_back(new MemberDelegateSig(target, func));
		}

		template <typename Func, typename UserClass>
		void _Bind(UserClass* const target, Func func)
		{
			using RawDelegateSig = RawDelegate<void, UserClass, Func, Params...>;
			this->Member_Binds.push_back(new RawDelegateSig(target, func));
		}

		template <typename DelSig, typename UserClass, typename Func>
		void _UnBind(UserClass* const target, Func func)
		{
			int index = 0;
			do
			{
				index = FindBind<DelSig>(target, func, index);
				RemoveAt(index);

			} while (index >= 0);
		}

		template <typename DelSig, typename Func, typename UserClass>
		void _UnBindSingle(UserClass* const target, Func func)
		{
			int index = FindBind<DelSig>(target, func);
			RemoveAt(index);
		}

		template <typename DelSig, typename UserClass, typename Func>
		bool _Contains(UserClass* const target, Func func) const
		{
			for (unsigned i = 0; i < this->Member_Binds.size(); ++i)
			{
				auto& Bind = Member_Binds[i];

				if (target != nullptr && target == Bind->GetObjectPointer())
				{
					auto bind = static_cast<DelSig*>(Bind);

					if (bind->GetFunctionPtr() == func)
					{
						return true;
					}
				}
			}
			return false;
		}

	public:
		//* Calls binded functions. Automatically removes invalid binds.
		void Broadcast(Params... in)
		{
			for (unsigned i = 0; i < this->Member_Binds.size(); )
			{
				auto& bind = Member_Binds[i];

				if (bind->IsValid())
				{
					bind->Execute(std::forward<Params>(in)...);
					++i;
				}
				else
				{
					Remove(bind);
				}
			}
		}

		//* Calls binded functions.
		void Broadcast(Params... in) const
		{
			for (unsigned i = 0; i < this->Member_Binds.size(); ++i)
			{
				auto& bind = Member_Binds[i];

				if (bind->IsValid())
				{
					bind->Execute(std::forward<Params>(in)...);
				}
			}
		}

		//Deletes all binds.
		void Clear()
		{
			for (auto& bind : this->Member_Binds)
			{
				delete bind;
			}
			this->Member_Binds.clear();
		}

		//*Broadcast
		inline void operator()(Params... in)
		{
			this->Broadcast(std::forward<Params>(in)...);
		}

#pragma region FreeBinds
		//*Binds method provided that it is not already bound.
		void AddBindUnique(FreeFunc func)
		{
			using FreeFunctionSig = FreeDelegate<FreeFunc, void, Params...>;

			if (ContainsBind(func) == false)
			{
				this->Member_Binds.push_back(new FreeFunctionSig(func));
			}
		}

		//*Binds method. Allows duplicates.
		inline void AddBind(FreeFunc func)
		{
			using FreeFunctionSig = FreeDelegate<FreeFunc, void, Params...>;
			this->Member_Binds.push_back(new FreeFunctionSig(func));
		}

		//*UnBinds the first bind that matches the function signature.
		void RemoveBindSingle(FreeFunc func)
		{
			int index = FindBind(func);
			RemoveAt(index);
		}

		//*UnBinds all methods matching the function signature.
		void RemoveBind(FreeFunc func)
		{
			int index = 0;
			do
			{
				index = FindBind(func, index);
				RemoveAt(index);

			} while (index >= 0);
		}

		//@Return: True if method is bound; False, if not.
		bool ContainsBind(FreeFunc func) const
		{
			using FreeFunctionSig = FreeDelegate<FreeFunc, void, Params...>;

			for (unsigned i = 0; i < this->Member_Binds.size(); )
			{
				auto bind = dynamic_cast<FreeFunctionSig*>(Member_Binds[i]);

				if (bind != nullptr && bind->GetFunctionPtr() == func)
				{
					return true;
				}
				++i;
			}
			return false;
		}

#pragma endregion

#pragma region RawBinds
		//*Binds method. Allows duplicates.
		template <typename UserClass>
		inline void AddBindUnique(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionSignature func)
		{
			_BindUnique(target, func);
		}

		//*Binds method. Allows duplicates.
		template <typename UserClass>
		inline void AddBindUnique(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionConstSignature func)
		{
			_BindUnique(target, func);
		}

		//*Binds method. Allows duplicates.
		template <typename UserClass>
		inline void AddBind(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionSignature func)
		{
			_Bind(target, func);
		}

		//*Binds method. Allows duplicates.
		template <typename UserClass>
		inline void AddBind(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionConstSignature func)
		{
			_Bind(target, func);
		}

		//*UnBinds all methods matching the function signature and object instance.
		template <typename UserClass>
		inline void RemoveBind(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionSignature func)
		{
			using Func = typename MFSig<UserClass>::MemberFunctionSignature;
			_UnBind<RawDelegate<void, UserClass, Func, Params...>>(target, func);
		}

		//*UnBinds all methods matching the function signature and object instance.
		template <typename UserClass>
		inline void RemoveBind(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionConstSignature func)
		{
			using Func = typename MFSig<UserClass>::MemberFunctionConstSignature;
			_UnBind<RawDelegate<void, UserClass, Func, Params...>>(target, func);
		}

		//*UnBinds the first bind that matches the function signature and object instance.
		template <typename UserClass>
		inline void RemoveBindSingle(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionSignature func)
		{
			using Func = typename MFSig<UserClass>::MemberFunctionSignature;
			_UnBindSingle<RawDelegate<void, UserClass, Func, Params...>>(target, func);
		}

		//*UnBinds the first bind that matches the function signature and object instance.
		template <typename UserClass>
		inline void RemoveBindSingle(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionConstSignature func)
		{
			using Func = typename MFSig<UserClass>::MemberFunctionConstSignature;
			_UnBindSingle<RawDelegate<void, UserClass, Func, Params...>>(target, func);
		}

		//@Return: True if object and method is bound; False, if not.
		template <typename UserClass>
		inline bool ContainsBind(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionSignature func) const
		{
			using Func = typename MFSig<UserClass>::MemberFunctionSignature;
			return _Contains<RawDelegate<void, UserClass, Func, Params...>>(target, func);
		}

		//@Return: True if object and method is bound; False, if not.
		template <typename UserClass>
		inline bool ContainsBind(UserClass* const& target, typename MFSig<UserClass>::MemberFunctionConstSignature func) const
		{
			using Func = typename MFSig<UserClass>::MemberFunctionConstSignature;
			return _Contains<RawDelegate<void, UserClass, Func, Params...>>(target, func);
		}

		//UnBinds all methods of object instance.
		template <typename UserClass>
		inline void RemoveBindAllInstance(UserClass* const& target)
		{
			_RemoveBindAllInstance(target);
		}

#pragma endregion

#pragma region SP Binds
		//*Binds method provided that it is not already bound.
		template <typename UserClass>
		inline void AddBindUnique(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionSignature func)
		{
			_BindUnique(target, func);
		}

		//*Binds method provided that it is not already bound.
		template <typename UserClass>
		inline void AddBindUnique(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionConstSignature func)
		{
			_BindUnique(target, func);
		}

		//*Binds method. Allows duplicates.
		template <typename UserClass>
		inline void AddBind(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionSignature func)
		{
			_Bind(target, func);
		}

		//*Binds method. Allows duplicates.
		template <typename UserClass>
		inline void AddBind(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionConstSignature func)
		{
			_Bind(target, func);
		}

		//*UnBinds all methods matching the function signature and object instance.
		template <typename UserClass>
		inline void RemoveBind(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionSignature func)
		{
			using Func = typename MFSig<UserClass>::MemberFunctionSignature;
			_UnBind<MemberDelegate<void, UserClass, Func, Params...>>(target.get(), func);
		}

		//*UnBinds all methods matching the function signature and object instance.
		template <typename UserClass>
		inline void RemoveBind(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionConstSignature func)
		{
			using Func = typename MFSig<UserClass>::MemberFunctionConstSignature;
			_UnBind<MemberDelegate<void, UserClass, Func, Params...>>(target.get(), func);
		}

		//*UnBinds the first bind that matches the function signature and object instance.
		template <typename UserClass>
		inline void RemoveBindSingle(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionSignature func)
		{
			using Func = typename MFSig<UserClass>::MemberFunctionSignature;
			_UnBindSingle<MemberDelegate<void, UserClass, Func, Params...>, Func, UserClass>(target.get(), func);
		}

		//*UnBinds the first bind that matches the function signature and object instance.
		template <typename UserClass>
		inline void RemoveBindSingle(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionConstSignature func)
		{
			using Func = typename MFSig<UserClass>::MemberFunctionConstSignature;
			_UnBindSingle<MemberDelegate<void, UserClass, Func, Params...>>(target.get(), func);
		}

		//@Return: True if object and method is bound; False, if not.
		template <typename UserClass>
		inline bool ContainsBind(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionSignature func) const
		{
			using Func = typename MFSig<UserClass>::MemberFunctionSignature;
			return _Contains<MemberDelegate<void, UserClass, Func, Params...>>(target.get(), func);
		}

		//@Return: True if object and method is bound; False, if not.
		template <typename UserClass>
		inline bool ContainsBind(std::shared_ptr<UserClass>& target, typename MFSig<UserClass>::MemberFunctionConstSignature func) const
		{
			using Func = typename MFSig<UserClass>::MemberFunctionConstSignature;
			return _Contains<MemberDelegate<void, UserClass, Func, Params...>>(target.get(), func);
		}

		//UnBinds all methods of object instance.
		template <typename UserClass>
		inline void RemoveBindAllInstance(std::shared_ptr<UserClass>& target)
		{
			_RemoveBindAllInstance(target.get());
		}

#pragma endregion

	};
#pragma endregion

}

#endif // !_DELEGATE_
