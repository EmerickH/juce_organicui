/*
  ==============================================================================

	ControllableEditor.cpp
	Created: 7 Oct 2016 2:04:37pm
	Author:  bkupe

  ==============================================================================
*/


ControllableEditor::ControllableEditor(Controllable * _controllable, bool isRoot) :
	InspectableEditor(_controllable, isRoot),
	controllable(_controllable),
	label("Label"),
    subContentHeight(0),
	minLabelWidth(160),
	minControlUIWidth(100),
	dragAndDropEnabled(true),
    showLabel(true)
{
	buildControllableUI();

	addAndMakeVisible(&label);
	label.setJustificationType(Justification::left);
	label.setFont(label.getFont().withHeight(GlobalSettings::getInstance()->fontSize->floatValue()));
	label.setText(controllable->niceName, dontSendNotification);
	label.setTooltip(ui->tooltip);
	label.addMouseListener(this, false);

	if (controllable->isRemovableByUser)
	{
		removeBT.reset(AssetManager::getInstance()->getRemoveBT());
		removeBT->addListener(this);
		addAndMakeVisible(removeBT.get());
	}

	if (controllable->canBeDisabledByUser)
	{
		enableBT.reset(AssetManager::getInstance()->getPowerBT());
		enableBT->addListener(this);
		enableBT->setToggleState(controllable->enabled, dontSendNotification);
		addAndMakeVisible(enableBT.get());
	}

	if (controllable->showWarningInUI)
	{
		warningUI.reset(new WarningTargetUI(controllable));
		warningUI->addComponentListener(this);
		addChildComponent(warningUI.get());
	}

	baseHeight = ui->getHeight();
	if (baseHeight == 0) baseHeight = GlobalSettings::getInstance()->fontSize->floatValue()+4;
	setSize(100, baseHeight);

	controllable->addAsyncControllableListener(this);
}

ControllableEditor::~ControllableEditor()
{
	if (controllable.wasObjectDeleted()) return;
	controllable->removeAsyncControllableListener(this);
}

void ControllableEditor::setShowLabel(bool value)
{
	if (showLabel == value) return;
	showLabel = value;
	if (showLabel)
	{
		addAndMakeVisible(&label);
	} else
	{
		removeChildComponent(&label);
	}

}

void ControllableEditor::buildControllableUI(bool resizeAfter)
{
	if (ui != nullptr) removeChildComponent(ui.get());
	ui.reset(controllable->createDefaultUI());
	ui->showLabel = false;
	ui->setOpaqueBackground(true);
	addAndMakeVisible(ui.get());

	if (resizeAfter) resized();
}

void ControllableEditor::resized()
{
	juce::Rectangle<int> r = getLocalBounds();
	r.removeFromBottom(subContentHeight);// .withHeight(16);

	if (warningUI != nullptr && warningUI->isVisible())
	{
		warningUI->setBounds(r.removeFromLeft(r.getHeight())); //warning
		r.removeFromLeft(2);
	}

	if (!controllable.wasObjectDeleted())
	{
		if (controllable->isRemovableByUser && removeBT != nullptr)
		{
			removeBT->setBounds(r.removeFromRight(r.getHeight()));
			r.removeFromRight(2);
		}


		if (controllable->canBeDisabledByUser && enableBT != nullptr)
		{
			enableBT->setBounds(r.removeFromLeft(r.getHeight()));
			r.removeFromLeft(2);
		}
	}

	resizedInternal(r);

	int controlSpace = jmax<int>(showLabel? jmin<int>(getWidth() - minLabelWidth, getWidth()*.75f) : getWidth(), minControlUIWidth);
	
	ui->setBounds(r.removeFromRight(controlSpace));
	
	if (showLabel)
	{
		r.removeFromRight(2); 
		label.setBounds(r);
	}

}

void ControllableEditor::mouseDown(const MouseEvent & e)
{
	InspectableEditor::mouseDown(e);

	if (e.mods.isRightButtonDown())
	{
		if (e.eventComponent == &label)
		{
			if (ui->showMenuOnRightClick) ui->showContextMenu();
		}
	}
}

void ControllableEditor::mouseDrag(const MouseEvent& e)
{
	InspectableEditor::mouseDrag(e);

	if (dragAndDropEnabled && e.eventComponent == &label)
	{
		var desc = var(new DynamicObject());
		desc.getDynamicObject()->setProperty("type", controllable->getTypeString());
		desc.getDynamicObject()->setProperty("dataType", "Controllable");
		//Image dragImage = this->createComponentSnapshot(this->getLocalBounds()).convertedToFormat(Image::ARGB);
		//dragImage.multiplyAllAlphas(.5f);
		Point<int> offset = -getMouseXYRelative();
		if (e.getDistanceFromDragStart() > 30) startDragging(desc, this, Image(), true, &offset);
	}
	
}

void ControllableEditor::newMessage(const Controllable::ControllableEvent & e)
{
	switch (e.type)
	{
	case Controllable::ControllableEvent::NAME_CHANGED:
		label.setText(controllable->niceName, dontSendNotification);
		break;
            
	case Controllable::ControllableEvent::STATE_CHANGED:
		if(enableBT != nullptr) enableBT->setToggleState(controllable->enabled, dontSendNotification);
		break;

	default:
		break;

	}
}

void ControllableEditor::componentVisibilityChanged(Component& c)
{
	InspectableEditor::componentVisibilityChanged(c);
	if (&c == warningUI.get()) resized();
}

void ControllableEditor::buttonClicked(Button * b)
{
	if (b == removeBT.get())
	{
		controllable->remove(true);
	}
	else if (b == enableBT.get())
	{
		controllable->setEnabled(!controllable->enabled);
	}
}
