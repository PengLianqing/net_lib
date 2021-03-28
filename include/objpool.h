//@Author Liu Yukang 
#pragma once
#include <type_traits>
#include "mempool.h"

namespace netco
{

	template<class T>
	class ObjPool
	{
	public:
		ObjPool() {};
		~ObjPool() {};

		DISALLOW_COPY_MOVE_AND_ASSIGN(ObjPool);

		template<typename... Args>
		inline T* new_obj(Args... args);

		inline void delete_obj(void* obj);

	private:
		template<typename... Args>
		inline T* new_aux(std::true_type, Args... args);

		template<typename... Args>
		inline T* new_aux(std::false_type, Args... args);

		inline void delete_aux(std::true_type, void* obj);

		inline void delete_aux(std::false_type, void* obj);

		MemPool<sizeof(T)> _memPool;

	};

	// integral_constant 类
	// 这个类是所有traits类的基类，分别提供了以下功能：
	// value_type 表示值的类型
	// value表示值
	// type 表示自己, 因此可以用::type::value来获取值
	// true_type和false_type两个特化类用来表示bool值类型的traits，很多traits类都需要继承它们

	template<class T>
	template<typename... Args>
	inline T* ObjPool<T>::new_obj(Args... args) // 创建一个新的对象
	{
		return new_aux(std::integral_constant<bool, std::is_trivially_constructible<T>::value>(), args...);
	}

	template<class T>
	template<typename... Args>
	inline T* ObjPool<T>::new_aux(std::true_type, Args... args) // 申请内存 true_type
	{
		// static_cast 把一个表达式转换为某种类型
		return static_cast<T*>(_memPool.AllocAMemBlock()); // 申请内存
	}

	template<class T>
	template<typename... Args>
	inline T* ObjPool<T>::new_aux(std::false_type, Args... args) // 申请内存 false_type
	{
		void* newPos = _memPool.AllocAMemBlock();
		return new(newPos) T(args...);
	}

	template<class T>
	inline void ObjPool<T>::delete_obj(void* obj) // 删除对象
	{
		if (!obj)
		{
			return;
		}
		delete_aux(std::integral_constant<bool, std::is_trivially_destructible<T>::value>(), obj);
	}

	template<class T>
	inline void ObjPool<T>::delete_aux(std::true_type, void* obj) // 释放内存 true_type
	{
		_memPool.FreeAMemBlock(obj);
	}

	template<class T>
	inline void ObjPool<T>::delete_aux(std::false_type, void* obj) // 释放内存 false_type
	{
		(static_cast<T*>(obj))->~T();
		_memPool.FreeAMemBlock(obj);
	}

}
