/*
  ==============================================================================

	Inspector.cpp
	Created: 9 May 2016 6:41:38pm
	Author:  bkupe

  ==============================================================================
*/

std::function<Inspector* (InspectableSelectionManager*)> InspectorUI::customCreateInspectorFunc = nullptr;

Inspector::Inspector(InspectableSelectionManager* _selectionManager) :
	selectionManager(nullptr),
	currentInspectable(nullptr),
	currentEditor(nullptr),
	showTextOnEmptyOrMulti(true)
{

	setSelectionManager(_selectionManager);

	vp.setScrollBarsShown(true, false);
	vp.setScrollOnDragMode(Viewport::ScrollOnDragMode::never);
	vp.setScrollBarThickness(10);
	addAndMakeVisible(vp);

	resized();
}



Inspector::~Inspector()
{
	if (selectionManager != nullptr) selectionManager->removeAsyncSelectionManagerListener(this);
	else if (InspectableSelectionManager::mainSelectionManager != nullptr) InspectableSelectionManager::mainSelectionManager->removeAsyncSelectionManagerListener(this);

	clear();
}


void Inspector::paint(Graphics& g)
{
	if (showTextOnEmptyOrMulti && currentInspectable == nullptr && selectionManager != nullptr)
	{
		g.setColour(Colours::white.withAlpha(.4f));
		g.setFont(jmin(getHeight() - 2, 14));
		String text = selectionManager->currentInspectables.size() == 0 ? "Select an object to edit its parameters here" : "Multi Editing is not supported right now, but keep trying :)";
		if (text.isNotEmpty()) g.drawFittedText(text, getLocalBounds(), Justification::centred, 3);
	}
}

void Inspector::resized()
{
	juce::Rectangle<int> r = getLocalBounds().reduced(3);
	resizedInternal(r);
}

void Inspector::resizedInternal(juce::Rectangle<int>& r)
{
	vp.setBounds(r);
	r.removeFromRight(10);

	if (currentEditor != nullptr)
	{
		if (!currentEditor->fitToContent) r.setHeight(currentEditor->getHeight());
		currentEditor->setSize(r.getWidth(), r.getHeight());
	}
}

void Inspector::setSelectionManager(InspectableSelectionManager* newSM)
{
	if (selectionManager != nullptr) selectionManager->removeAsyncSelectionManagerListener(this);

	selectionManager = newSM;

	if (selectionManager == nullptr) selectionManager = InspectableSelectionManager::mainSelectionManager;
	if (selectionManager != nullptr) selectionManager->addAsyncSelectionManagerListener(this);
}


void Inspector::setCurrentInspectable(WeakReference<Inspectable> inspectable, bool setInspectableSelection)
{
	if (!isEnabled()) return;

	if (inspectable == currentInspectable)
	{
		return;
	}

	MessageManagerLock mmLock;

	if (currentInspectable != nullptr)
	{
		if (!currentInspectable.wasObjectDeleted())
		{
			currentInspectable->removeInspectableListener(this);
			if (setInspectableSelection) currentInspectable->setSelected(false);
		}

		if (currentEditor != nullptr)
		{
			vp.setViewedComponent(nullptr);
			currentEditor = nullptr;
		}
	}
	currentInspectable = inspectable;

	if (currentInspectable.get() != nullptr)
	{
		if (setInspectableSelection) currentInspectable->setSelected(true);
		currentInspectable->addInspectableListener(this);
		currentEditor.reset(currentInspectable->getEditor(true));
	}

	vp.setViewedComponent(currentEditor.get(), false);
	resized();

	listeners.call(&InspectorListener::currentInspectableChanged, this);
}


void Inspector::clear()
{
	setCurrentInspectable(nullptr);
}

void Inspector::inspectableDestroyed(Inspectable* i)
{
	if (currentInspectable == i) setCurrentInspectable(nullptr);
}

void Inspector::newMessage(const InspectableSelectionManager::SelectionEvent& e)
{
	if (e.type == InspectableSelectionManager::SelectionEvent::SELECTION_CHANGED)
	{
		if (selectionManager->isEmpty())
		{
			if (curSelectionDoesNotAffectInspector) return;
			setCurrentInspectable(nullptr);
		}
		else
		{
			Inspectable* newI = selectionManager->currentInspectables[0];
			curSelectionDoesNotAffectInspector = !newI->showInspectorOnSelect;
			if (curSelectionDoesNotAffectInspector) return;

			if (selectionManager->currentInspectables.size() == 1) setCurrentInspectable(newI);
			else setCurrentInspectable(nullptr, false);
		}

		repaint();
	}
}

InspectorUI::InspectorUI(const String& name, InspectableSelectionManager* selectionManager) :
	ShapeShifterContentComponent(name)
{
	inspector.reset(customCreateInspectorFunc != nullptr ? customCreateInspectorFunc(selectionManager) : new Inspector(selectionManager));
	addAndMakeVisible(inspector.get());

	helpID = "Inspector";
}

InspectorUI::~InspectorUI()
{
}

void InspectorUI::resized()
{
	inspector->setBounds(getLocalBounds());
}
