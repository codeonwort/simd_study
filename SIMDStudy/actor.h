#pragma once

#include "int_types.h"

#include <vector>

class Component;

class Actor
{
public:
	void addComponent(Component* C)
	{
		components.push_back(C);
	}

	template<typename T>
	T* findComponent()
	{
		for (uint32 i = 0; i < components.size(); ++i)
		{
			if (dynamic_cast<T*>(components[i]) != nullptr)
			{
				return static_cast<T*>(components[i]);
			}
		}
		return nullptr;
	}
	
private:
	std::vector<Component*> components;

};
