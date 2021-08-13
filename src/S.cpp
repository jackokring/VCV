#include "plugin.hpp"


struct S : Module {
	enum ParamIds {
		BPM,
		DIV,
		NUM_PARAMS
	};
	enum InputIds {
		CLK,
		STRT,
		STOP,
		CONT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	double beatCounter = 0;
	int beatIn = 0;
	int divider = 0;
	bool running = false;

	dsp::SchmittTrigger tClk;
	dsp::SchmittTrigger tStrt;
	dsp::SchmittTrigger tStop;
	dsp::SchmittTrigger tCont;

	S() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(BPM, 0.f, 240.f, 120.f, "Estimated Tempo", " bpm");
		configParam(DIV, 1.f, 16.f, 1.f, "Division Ratio");
	}

	//double bit error ?? time??
	double modulo(double x, float m) {
		double div = x / m;
		long d = (long) div;
		double rem = x - d * m;
		return rem;
	}

	void process(const ProcessArgs& args) override {
		float fs = args.sampleRate;

		double bps = (double)params[BPM].getValue() / 15.f;//beat per bar
		double beatSamp = bps / fs;//beats per sample
		float beats = beatCounter;
		int div = (int)params[DIV].getValue();

		float trigClk = inputs[CLK].getVoltage();
		bool triggerClk = tClk.process(rescale(trigClk, 0.1f, 2.f, 0.f, 1.f));
		float trigStrt = inputs[STRT].getVoltage();
		bool triggerStrt = tStrt.process(rescale(trigStrt, 0.1f, 2.f, 0.f, 1.f));
		float trigStop = inputs[STOP].getVoltage();
		bool triggerStop = tStop.process(rescale(trigStop, 0.1f, 2.f, 0.f, 1.f));
		float trigCont = inputs[CONT].getVoltage();
		bool triggerCont = tCont.process(rescale(trigCont, 0.1f, 2.f, 0.f, 1.f));

		if(triggerStrt) {
			beatCounter = 0.f;
			beatIn = 0;
			running = true;
		}
		if(triggerCont) {
			running = true;
		}
		if(triggerStop) {
			running = false;
		}
		if(triggerClk && running) {//running check
			div++;
			if(divider >= div) {
				div = 0;
				beatIn++;
				beatIn &= 63;
				beatCounter = 0.f;
			}
		}

		if(running) beatCounter += beatSamp;
		if(beats >= 1) {//sanity range before use
			beatCounter = modulo(beatCounter, 1);//beats long
		}

		outputs[OUT].setVoltage((beatIn + beatCounter) * 10.f / 64.f);

	}
};

//geometry edit
#define HP 3
#define LANES 1
#define RUNGS 7

//ok
#define HP_UNIT 5.08
#define WIDTH (HP*HP_UNIT)
#define X_SPLIT (WIDTH / 2.f / LANES)

#define HEIGHT 128.5
#define Y_MARGIN 0.05f
#define R_HEIGHT (HEIGHT*(1-2*Y_MARGIN))
#define Y_SPLIT (R_HEIGHT / 2.f / RUNGS)

//placement macro
#define loc(x,y) mm2px(Vec(X_SPLIT*(1+2*(x-1)), (HEIGHT*Y_MARGIN)+Y_SPLIT*(1+2*(y-1))))

struct SWidget : ModuleWidget {
	SWidget(S* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/S.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(loc(1, 1), module, S::BPM));
		addParam(createParamCentered<RoundBlackSnapKnob>(loc(1, 2), module, S::DIV));

		addInput(createInputCentered<PJ301MPort>(loc(1, 3), module, S::CLK));
		addInput(createInputCentered<PJ301MPort>(loc(1, 4), module, S::STRT));
		addInput(createInputCentered<PJ301MPort>(loc(1, 5), module, S::STOP));
		addInput(createInputCentered<PJ301MPort>(loc(1, 6), module, S::CONT));

		addOutput(createOutputCentered<PJ301MPort>(loc(1, 7), module, S::OUT));
	}
};


Model* modelS = createModel<S, SWidget>("S");