/*
  ==============================================================================

	Factory.h
	Created: 7 May 2017 2:43:16pm
	Author:  Ben

  ==============================================================================
*/

#pragma once

template<class T>
class BaseFactoryDefinition
{
public:
	BaseFactoryDefinition(juce::StringRef menuPath, juce::StringRef type) :
		type(type),
		menuPath(menuPath)
	{
	}

	virtual ~BaseFactoryDefinition() {}

	juce::String type;
	juce::String menuPath;
	juce::Image icon;

	BaseFactoryDefinition* addIcon(juce::Image icon)
	{
		this->icon = icon;
		return this;
	}

	virtual T* create() { jassertfalse; return new T(); }
};

template<class T, typename CreateFunc>
class FactoryDefinition :
	public BaseFactoryDefinition<T>
{
public:
	FactoryDefinition(juce::StringRef menuPath, juce::StringRef type, CreateFunc createFunc) :
		BaseFactoryDefinition<T>(menuPath, type),
		createFunc(createFunc)
	{}

	virtual ~FactoryDefinition() {}

	CreateFunc createFunc;

	static FactoryDefinition* createDef(juce::StringRef menu, juce::StringRef type, CreateFunc createFunc)
	{
		FactoryDefinition* d = new FactoryDefinition(menu, type, createFunc);
		return d;
	}
};

template<class T, typename CreateFunc>
class FactoryParametricDefinition :
	public FactoryDefinition<T, CreateFunc>
{
public:
	FactoryParametricDefinition(juce::StringRef menuPath, juce::StringRef type, CreateFunc createFunc, juce::var params = new juce::DynamicObject()) :
		FactoryDefinition<T, CreateFunc>(menuPath, type, createFunc),
		params(params)
	{
		this->params.getDynamicObject()->setProperty("type", juce::String(type));
	}

	virtual ~FactoryParametricDefinition() {}

	juce::var params;

	FactoryParametricDefinition* addParam(const juce::String& paramName, const juce::var& value)
	{
		params.getDynamicObject()->setProperty(paramName, value);
		return this; //daisy-chain
	}
};

template<class T>
class FactorySimpleParametricDefinition :
	public FactoryParametricDefinition<T, std::function<T *(juce::var)>>
{
public:
	FactorySimpleParametricDefinition(juce::StringRef menuPath, juce::StringRef type, std::function<T* (juce::var)> createFunc, juce::var params = new juce::DynamicObject()) :
		FactoryParametricDefinition<T, std::function<T* (juce::var)>>(menuPath, type, createFunc, params)
	{
	}

	virtual ~FactorySimpleParametricDefinition() {}

	virtual T* create() override {
		return this->createFunc(this->params);
	}


	static FactorySimpleParametricDefinition* createDef(juce::StringRef menu, juce::StringRef type, std::function<T* (juce::var)> createFunc, juce::var params = new juce::DynamicObject())
	{
		return new FactorySimpleParametricDefinition(menu, type, createFunc, params);
	}

	template<class S>
	static FactorySimpleParametricDefinition* createDef(juce::StringRef menu)
	{
		return createDef(menu, S::getTypeStringStatic(), &FactorySimpleParametricDefinition<T>::createTemplated<S>, new juce::DynamicObject());
	}

	template<class S>
	static FactorySimpleParametricDefinition* createDef(juce::StringRef menu, juce::StringRef type, juce::var params = new juce::DynamicObject())
	{
		return createDef(menu, type, &FactorySimpleParametricDefinition<T>::createTemplated<S>, params);
	}

	template<class S>
	static T* createTemplated(juce::var params = new juce::DynamicObject()) { return new S(params); }

};


template<class T>
class Factory
{
public:
	Factory() {}
	virtual ~Factory() {}

	juce::OwnedArray<BaseFactoryDefinition<T>> defs;
	juce::PopupMenu menu;

	virtual void buildPopupMenu(int startOffset = 0)
	{
		menu.clear();
		juce::OwnedArray<juce::PopupMenu> subMenus;
		juce::Array<juce::String> subMenuNames;

		for (auto& d : defs)
		{
			int itemID = defs.indexOf(d) + 1 + startOffset;//start at 1+offset for menu

			if (d->menuPath.isEmpty())
			{
				menu.addItem(itemID, d->type, true, false, d->icon);
				continue;
			}

			int subMenuIndex = -1;

			for (int i = 0; i < subMenus.size(); ++i)
			{
				if (subMenuNames[i] == d->menuPath)
				{
					subMenuIndex = i;
					break;
				}
			}
			if (subMenuIndex == -1)
			{
				subMenuNames.add(d->menuPath);
				subMenus.add(new juce::PopupMenu());
				subMenuIndex = subMenus.size() - 1;
			}

			subMenus[subMenuIndex]->addItem(itemID, d->type, true, false, d->icon);
		}

		for (int i = 0; i < subMenus.size(); ++i) menu.addSubMenu(subMenuNames[i], *subMenus[i]);
	}

	virtual void showCreateMenu(std::function<void(T*)> returnFunc)
	{
		if (defs.size() == 1)
		{
			if (T* r = this->createFromMenuResult(1)) returnFunc(r);
			return;
		}

		getMenu().showMenuAsync(juce::PopupMenu::Options(), [this, returnFunc](int result)
			{
				if (T* r = this->createFromMenuResult(result)) returnFunc(r);
			}
		);
	}

	virtual juce::PopupMenu getMenu(int startOffset = 0)
	{
		if (menu.getNumItems() == 0) buildPopupMenu(startOffset);
		return menu;
	}

	virtual T* createFromMenuResult(int result)
	{
		if (result <= 0 || result > defs.size()) return nullptr;
		else
		{
			BaseFactoryDefinition<T> * d = defs[result - 1];//result 0 is no result
			return create(d);
		}
	}

	virtual T* create(const juce::String& type)
	{
		for (auto& d : defs)
		{
			if (d->type == type) return create(d);
		}

		LOGERROR("Type " << type << " does not exist for this manager.");
		return nullptr;
	}

	virtual T* create(BaseFactoryDefinition<T> * def)
	{
		return def->create();
	}

	bool hasDefinitionWithType(const juce::String& type)
	{
		for (auto& d : defs)
		{
			if (d->type == type) return true;
		}

		return false;
	}

	typedef FactorySimpleParametricDefinition<T> Definition;
};
