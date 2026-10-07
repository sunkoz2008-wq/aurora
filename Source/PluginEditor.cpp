#include "PluginEditor.h"

using namespace juce;

namespace
{
    String sectionTitle (const String& prefix)
    {
        if (prefix == "io")  return "I/O";
        if (prefix == "eq")  return "EQ 3 bandes";
        if (prefix == "sat") return "Saturation (OS 2x)";
        if (prefix == "cho") return "Chorus";
        if (prefix == "dly") return "Delay";
        if (prefix == "rev") return "Reverb";
        if (prefix == "cmp") return "Compresseur";
        if (prefix == "pan") return "Auto-Pan / Largeur";
        return prefix;
    }

    constexpr int cellW = 80;
    constexpr int rowH = 180;
}

AuroraAudioProcessorEditor::AuroraAudioProcessorEditor (AuroraAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    const auto accent = Colour (0xff6ee7ff);

    for (auto* ap : proc.getParameters())
    {
        auto* param = dynamic_cast<RangedAudioParameter*> (ap);
        if (param == nullptr)
            continue;

        const auto prefix = param->paramID.upToFirstOccurrenceOf ("_", false, false);

        Section* sec = nullptr;
        for (auto& s : sections)
            if (s->prefix == prefix)
                sec = s.get();

        if (sec == nullptr)
        {
            sections.push_back (std::make_unique<Section>());
            sec = sections.back().get();
            sec->prefix = prefix;
            sec->title = sectionTitle (prefix);
        }

        auto cell = std::make_unique<Cell>();
        cell->label.setText (param->getName (32), dontSendNotification);
        cell->label.setJustificationType (Justification::centred);
        addAndMakeVisible (cell->label);

        if (auto* choice = dynamic_cast<AudioParameterChoice*> (param))
        {
            cell->combo = std::make_unique<ComboBox>();
            cell->combo->addItemList (choice->choices, 1);
            addAndMakeVisible (*cell->combo);
            cell->comboAtt = std::make_unique<ComboBoxParameterAttachment> (*param, *cell->combo);
        }
        else if (dynamic_cast<AudioParameterBool*> (param) != nullptr)
        {
            cell->toggle = std::make_unique<ToggleButton> (param->getName (32));
            cell->toggle->setColour (ToggleButton::tickColourId, accent);
            addAndMakeVisible (*cell->toggle);
            cell->toggleAtt = std::make_unique<ButtonParameterAttachment> (*param, *cell->toggle);
        }
        else
        {
            cell->slider = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
            cell->slider->setTextBoxStyle (Slider::TextBoxBelow, false, 74, 16);
            cell->slider->setColour (Slider::rotarySliderFillColourId, accent);
            cell->slider->setColour (Slider::thumbColourId, Colours::white);
            cell->slider->textFromValueFunction = [param] (double v)
            {
                return (param->getText (param->convertTo0to1 ((float) v), 12) + " " + param->getLabel()).trim();
            };
            cell->slider->valueFromTextFunction = [param] (const String& t)
            {
                return (double) param->convertFrom0to1 (param->getValueForText (t));
            };
            addAndMakeVisible (*cell->slider);
            cell->sliderAtt = std::make_unique<SliderParameterAttachment> (*param, *cell->slider);
            cell->slider->setDoubleClickReturnValue (true, (double) param->convertFrom0to1 (param->getDefaultValue()));
            cell->slider->updateText();
        }

        sec->cells.push_back (cell.get());
        cells.push_back (std::move (cell));
    }

    setSize (1140, 600);
}

void AuroraAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff10131a));

    g.setColour (Colours::white);
    g.setFont (26.f);
    g.drawText ("AURORA", 16, 8, 300, 34, Justification::centredLeft);
    g.setColour (Colour (0xff8a93a6));
    g.setFont (13.f);
    g.drawText ("multi-effet stereo", 150, 8, 300, 34, Justification::centredLeft);

    for (auto& s : sections)
    {
        g.setColour (Colour (0xff1c2230));
        g.fillRoundedRectangle (s->area.toFloat(), 8.f);
        g.setColour (Colour (0xff6ee7ff));
        g.setFont (14.f);
        g.drawText (s->title, s->area.getX() + 10, s->area.getY() + 4, s->area.getWidth() - 20, 20,
                    Justification::centredLeft);
    }
}

void AuroraAudioProcessorEditor::resized()
{
    int x = 10, y = 50;

    for (auto& s : sections)
    {
        const int w = (int) s->cells.size() * cellW + 16;
        if (x + w > getWidth() - 10 && x > 10)
        {
            x = 10;
            y += rowH;
        }

        s->area = { x, y, w, rowH - 10 };

        int cx = x + 8;
        for (auto* c : s->cells)
        {
            c->label.setBounds (cx, y + 24, cellW, 18);
            if (c->slider != nullptr)
                c->slider->setBounds (cx, y + 42, cellW, 120);
            else if (c->combo != nullptr)
                c->combo->setBounds (cx + 4, y + 80, cellW - 8, 24);
            else if (c->toggle != nullptr)
                c->toggle->setBounds (cx + 4, y + 80, cellW - 4, 24);
            cx += cellW;
        }

        x += w + 8;
    }
}
