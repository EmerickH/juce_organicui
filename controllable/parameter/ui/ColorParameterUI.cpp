/*
  ==============================================================================

	ColorParameterUI.cpp
	Created: 11 Apr 2017 10:42:03am
	Author:  Ben

  ==============================================================================
*/

#include "JuceHeader.h"
#include "ColorParameterUI.h"

ColorParameterUI::ColorParameterUI(Array<ColorParameter*> parameters) :
	ParameterUI(Inspectable::getArrayAs<ColorParameter, Parameter>(parameters), ORGANICUI_DEFAULT_TIMER),
	colorParams(parameters),
	colorParam(parameters[0]),
	dispatchOnDoubleClick(true),
	colorEditor(nullptr),
	dispatchOnSingleClick(false)
{
	setSize(200, GlobalSettings::getInstance()->fontSize->floatValue() + 4);//default size
	showLabel = false;
}

ColorParameterUI::~ColorParameterUI()
{
	if (colorEditor != nullptr) colorEditor->removeComponentListener(this);

}

void ColorParameterUI::paint(Graphics& g)
{
	if (shouldBailOut()) return;


	Rectangle<int> r = getLocalBounds();
	if (r.getWidth() < 2 || getHeight() < 2) return;

	Colour c = colorParam->getColor();
	int size = jmin(getWidth(), getHeight()) / 2;
	if (!c.isOpaque()) g.fillCheckerBoard(r.reduced(1).toFloat(), size, size, Colours::white, Colours::white.darker(.2f));

	g.setColour(c);
	g.fillRoundedRectangle(r.toFloat(), 2);

	if (isInteractable())
	{
		g.setColour(c.brighter(.5f).withAlpha(1.0f));
		g.drawRoundedRectangle(r.reduced(1).toFloat(), 2, 2);
	}

	if (showLabel)
	{
		Rectangle<int> tr = r.reduced(2);
		g.setFont(FontOptions(jlimit(12, 40, jmin(tr.getHeight(), tr.getWidth()) - 16)));
		g.setColour(useCustomTextColor ? customTextColor : TEXT_COLOR);
		g.drawFittedText(customLabel.isNotEmpty() ? customLabel : colorParam->niceName, tr, Justification::centred, 1);
	}

}

void ColorParameterUI::resized()
{

}

void ColorParameterUI::mouseDownInternal(const MouseEvent& e)
{
	if (dispatchOnSingleClick) showEditWindow();
}

void ColorParameterUI::showEditWindowInternal()
{
	if (!isInteractable()) return;
	if (shouldBailOut()) return;

	std::unique_ptr<OrganicColorPicker> selector(new OrganicColorPicker(this));
	selector->setName("Color for " + parameter->niceName);
	selector->setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
	selector->setColour(ColourSelector::labelTextColourId, TEXT_COLOR);
	selector->setSize(300, 400);
	valueOnEditorOpen = colorParam->getValue();
	colorEditor = &CallOutBox::launchAsynchronously(std::move(selector), getScreenBounds(), nullptr);
}

void ColorParameterUI::showEditRangeWindowInternal()
{
	if (!parameter->isCustomizableByUser) return;

	AlertWindow* nameWindow = new AlertWindow("Change color bounds", "Set new bounds for this parameter", AlertWindow::AlertIconType::NoIcon, this);

	const String coordNames[4]{ "Red", "Green", "Blue", "Alpha" };

	for (int i = 0; i < 4; ++i)
	{
		nameWindow->addTextEditor("minVal" + String(i), String((float)colorParam->minimumValue[i]), "Minimum " + coordNames[i]);
		nameWindow->addTextEditor("maxVal" + String(i), String((float)colorParam->maximumValue[i]), "Maximum " + coordNames[i]);
	}

	nameWindow->addButton("OK", 1, KeyPress(KeyPress::returnKey));
	nameWindow->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	nameWindow->enterModalState(true, ModalCallbackFunction::create([this, nameWindow](int result)
		{
			if (result != 1) return;

			float newMins[4];
			float newMaxs[4];
			for (int i = 0; i < 4; ++i)
			{
				newMins[i] = nameWindow->getTextEditorContents("minVal" + String(i)).getFloatValue();
				newMaxs[i] = nameWindow->getTextEditorContents("maxVal" + String(i)).getFloatValue();
			}
			colorParam->setBounds(newMins[0], newMins[1], newMins[2], newMins[3],
				jmax(newMins[0], newMaxs[0]),
				jmax(newMins[1], newMaxs[1]),
				jmax(newMins[2], newMaxs[2]),
				jmax(newMins[3], newMaxs[3]));
		}
	), true);
}

void ColorParameterUI::componentBeingDeleted(Component& c)
{
	if (&c == colorEditor)
	{
		colorParam->setUndoableValue(colorParam->value, false, true);
		colorEditor = nullptr;
	}
}

void ColorParameterUI::valueChanged(const var&)
{
	shouldRepaint = true;
}


OrganicColorPicker::OrganicColorPicker(ColorParameterUI* colorParamUI) :
	Component("ColorPicker"),
	colorParamUI(colorParamUI),
	r("R", "Red", 0, 0, 1),
	g("G", "Green", 0, 0, 1),
	b("B", "Blue", 0, 0, 1),
	a("A", "Alpha", 0, 0, 1),
	h("H", "Hue", 0, 0, 1),
	s("S", "Saturation", 0, 0, 1),
	bri("B", "Brightness", 0, 0, 1),
	hex("Hex", "Hex", "#"),
	isUpdatingColor(false),
	isDraggingHueSat(false)
{
	colorParamUI->parameter->addAsyncParameterListener(this);

	hueSatHandle.reset(new HueSatHandle(this));
	addAndMakeVisible(hueSatHandle.get());

	rUI.reset(r.createSlider());
	gUI.reset(g.createSlider());
	bUI.reset(b.createSlider());
	aUI.reset(a.createSlider());
	hUI.reset(h.createSlider());
	sUI.reset(s.createSlider());
	briUI.reset(bri.createSlider());
	hexUI.reset(hex.createStringParameterUI());
	hueSatHandle.reset(new HueSatHandle(this));

	rUI->useCustomBGColor = true;
	rUI->customBGColor = Colours::red.darker();
	rUI->useCustomFGColor = true;
	rUI->customFGColor = Colours::red.brighter();

	gUI->useCustomBGColor = true;
	gUI->customBGColor = Colours::green.darker();
	gUI->useCustomFGColor = true;
	gUI->customFGColor = Colours::green.brighter();

	bUI->useCustomBGColor = true;
	bUI->customBGColor = Colours::blue.darker();
	bUI->useCustomFGColor = true;
	bUI->customFGColor = Colours::blue.brighter();


	aUI->useCustomBGColor = true;
	aUI->customBGColor = Colours::lightgrey;
	aUI->useCustomTextColor = true;
	aUI->customTextColor = Colours::white;


	addAndMakeVisible(rUI.get());
	addAndMakeVisible(gUI.get());
	addAndMakeVisible(bUI.get());
	addAndMakeVisible(aUI.get());
	addAndMakeVisible(hUI.get());
	addAndMakeVisible(sUI.get());
	addAndMakeVisible(briUI.get());
	addAndMakeVisible(hexUI.get());
	addAndMakeVisible(hueSatHandle.get());

	rUI->addMouseListener(this, false);
	gUI->addMouseListener(this, false);
	bUI->addMouseListener(this, false);
	aUI->addMouseListener(this, false);
	hUI->addMouseListener(this, false);
	sUI->addMouseListener(this, false);
	briUI->addMouseListener(this, false);
	hexUI->addMouseListener(this, false);
	hueSatHandle->addMouseListener(this, false);

	r.addAsyncParameterListener(this);
	g.addAsyncParameterListener(this);
	b.addAsyncParameterListener(this);
	a.addAsyncParameterListener(this);
	h.addAsyncParameterListener(this);
	s.addAsyncParameterListener(this);
	bri.addAsyncParameterListener(this);
	hex.addAsyncParameterListener(this);


	setSize(300, 400);
	updateFromParameter();
}

OrganicColorPicker::~OrganicColorPicker()
{
	if (colorParamUI->colorParam == nullptr) return;
	colorParamUI->colorParam->removeAsyncParameterListener(this);
}

void OrganicColorPicker::paint(Graphics& g)
{
	g.fillAll(Colours::transparentBlack);

	g.fillRect(hueSatRect);
	g.setColour(Colours::white.withAlpha(.5f));
	g.drawRoundedRectangle(hueSatRect.toFloat(), 2.f, 2.f);
	g.drawImageAt(hueSatImage, hueSatRect.getX(), hueSatRect.getY());
}

void OrganicColorPicker::resized()
{
	Rectangle<int> r = getLocalBounds().reduced(2);
	hueSatRect = r.removeFromTop(r.getHeight() / 2);
	r.removeFromTop(4);
	rUI->setBounds(r.removeFromTop(15));
	r.removeFromTop(4);
	gUI->setBounds(r.removeFromTop(15));
	r.removeFromTop(4);
	bUI->setBounds(r.removeFromTop(15));
	r.removeFromTop(4);
	aUI->setBounds(r.removeFromTop(15));
	r.removeFromTop(4);
	hUI->setBounds(r.removeFromTop(15));
	r.removeFromTop(4);
	sUI->setBounds(r.removeFromTop(15));
	r.removeFromTop(4);
	briUI->setBounds(r.removeFromTop(15));

	hexUI->setBounds(getLocalBounds().removeFromBottom(20));

	regenerateHueSatImage();
}

void OrganicColorPicker::mouseDown(const MouseEvent& e)
{
	if (hueSatRect.contains(getMouseXYRelative()) || e.eventComponent == hueSatHandle.get())
	{
		updateHueSat();
		isDraggingHueSat = true;
	}
}

void OrganicColorPicker::mouseDrag(const MouseEvent& e)
{
	if (isDraggingHueSat) updateHueSat();
}

void OrganicColorPicker::mouseUp(const MouseEvent& e)
{

	if (e.eventComponent == hueSatHandle.get() ||
		e.eventComponent == rUI.get() ||
		e.eventComponent == gUI.get() ||
		e.eventComponent == bUI.get() ||
		e.eventComponent == aUI.get() ||
		e.eventComponent == hUI.get() ||
		e.eventComponent == sUI.get() ||
		e.eventComponent == briUI.get() ||
		isDraggingHueSat)
	{
		setUndoableParam();
	}

	isDraggingHueSat = false;
}

void OrganicColorPicker::regenerateHueSatImage()
{
	if (hueSatImage.isValid() && hueSatImage.getWidth() == hueSatRect.getWidth() && hueSatImage.getHeight() == hueSatRect.getHeight()) return;
	hueSatImage = Image(Image::PixelFormat::ARGB, hueSatRect.getWidth(), hueSatRect.getHeight(), true);
	Image::BitmapData bd(hueSatImage, Image::BitmapData::ReadWriteMode::writeOnly);
	for (int x = 0; x < hueSatRect.getWidth(); ++x)
	{
		for (int y = 0; y < hueSatRect.getHeight(); ++y)
		{
			float hue = static_cast<float>(x) / hueSatRect.getWidth();
			float saturation = static_cast<float>(y) / hueSatRect.getHeight();
			Colour c = Colour::fromHSV(hue, saturation, 1.0f, 1.0f);
			bd.setPixelColour(x, y, c);
		}
	}
}

void OrganicColorPicker::updateHueSat()
{
	if (isUpdatingColor) return;

	Point<int> p = getMouseXYRelative();
	p.setX(jlimit(hueSatRect.getX(), hueSatRect.getRight(), p.getX()));
	p.setY(jlimit(hueSatRect.getY(), hueSatRect.getBottom(), p.getY()));
	//hueSatHandle->setCentrePosition(p);

	Point<float> relHS = Point<float>(p.getX() - hueSatRect.getX(), p.getY() - hueSatRect.getY()) / Point<float>(hueSatRect.getWidth(), hueSatRect.getHeight());

	updateFromHSV(relHS.x, relHS.y, bri.floatValue());
}

void OrganicColorPicker::setEditingColor(const juce::Colour& c, bool setRGB, bool setHSV, bool setParam)
{
	if (isUpdatingColor) return;
	isUpdatingColor = true;

	if (setHSV) {
		h.setValue(c.getHue());
		s.setValue(c.getSaturation());
		bri.setValue(c.getBrightness());
	}

	if (setRGB) {
		r.setValue(c.getFloatRed());
		g.setValue(c.getFloatGreen());
		b.setValue(c.getFloatBlue());
	}


	updateHueSatHandle();
	hex.setValue(c.toDisplayString(true));
	a.setValue(c.getFloatAlpha());

	if (setParam) colorParamUI->colorParam->setColor(c, false, false, true);

	isUpdatingColor = false;
}

void OrganicColorPicker::setUndoableParam()
{
	colorParamUI->colorParam->setUndoableColor(colorParamUI->colorParam->getColor(), true);
}

void OrganicColorPicker::updateFromRGB(float _r, float _g, float _b)
{
	if (isUpdatingColor) return;
	isUpdatingColor = true;
	r.setValue(_r);
	g.setValue(_g);
	b.setValue(_b);
	isUpdatingColor = false;

	setEditingColor(Colour::fromFloatRGBA(_r, _g, _b, a.floatValue()), false, true);
}

void OrganicColorPicker::updateFromHSV(float _h, float _s, float _v)
{
	if (isUpdatingColor) return;
	isUpdatingColor = true;
	h.setValue(_h);
	s.setValue(_s);
	bri.setValue(_v);
	isUpdatingColor = false;

	setEditingColor(Colour::fromHSV(_h, _s, _v, 1.0f).withAlpha(a.floatValue()), true, false);
}

void OrganicColorPicker::updateFromHex(String s)
{
	if (isUpdatingColor) return;

	setEditingColor(Colour::fromString(s));
	setUndoableParam();
}

void OrganicColorPicker::updateFromAlpha(float alpha)
{
	if (isUpdatingColor) return;
	setEditingColor(Colour::fromFloatRGBA(r.floatValue(), g.floatValue(), b.floatValue(), alpha));
}

void OrganicColorPicker::updateHueSatHandle()
{
	Point<int> relPos = hueSatRect.getRelativePoint(h.floatValue(), s.floatValue());
	hueSatHandle->setCentrePosition(hueSatRect.getX() + relPos.getX(), hueSatRect.getY() + relPos.getY());
}

void OrganicColorPicker::updateFromParameter()
{
	if (isUpdatingColor) return;
	setEditingColor(colorParamUI->colorParam->getColor());
}

void OrganicColorPicker::newMessage(const Parameter::ParameterEvent& e)
{
	if (isUpdatingColor) return;

	if (e.parameter == colorParamUI->parameter)
	{
		updateFromParameter();
	}
	else if (e.parameter == &r || e.parameter == &g || e.parameter == &b)
	{
		updateFromRGB(r.floatValue(), g.floatValue(), b.floatValue());
	}
	else if (e.parameter == &a)
	{
		updateFromAlpha(a.floatValue());
	}
	else if (e.parameter == &h || e.parameter == &s || e.parameter == &bri)
	{
		updateFromHSV(h.floatValue(), s.floatValue(), bri.floatValue());
	}

	else if (e.parameter == &hex)
	{
		updateFromHex(hex.stringValue());
	}
}

OrganicColorPicker::HueSatHandle::HueSatHandle(OrganicColorPicker* picker) :
	Component("HueSatHandle"),
	picker(picker)
{
	setSize(20, 20);
}

OrganicColorPicker::HueSatHandle::~HueSatHandle()
{
}

void OrganicColorPicker::HueSatHandle::paint(Graphics& g)
{
	//draw circle
	Rectangle<float> r = getLocalBounds().toFloat().reduced(4);
	Colour c = picker->colorParamUI->colorParam->getColor();
	g.setColour(c);
	g.fillEllipse(r);
	g.setColour(c.darker(.8f).withAlpha(.9f));
	g.drawEllipse(r, 2);
}

void OrganicColorPicker::HueSatHandle::resized()
{
}

void OrganicColorPicker::HueSatHandle::mouseDown(const MouseEvent& e)
{
}

void OrganicColorPicker::HueSatHandle::mouseDrag(const MouseEvent& e)
{
}

void OrganicColorPicker::HueSatHandle::mouseUp(const MouseEvent& e)
{
}
