/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  An ASCII XACT SoundBank, found in the non-Xbox versions of Jade Empire as _xsb.txt files.
 */

#include "src/common/strutil.h"
#include "src/common/readstream.h"
#include "src/common/streamtokenizer.h"

#include "src/sound/xactsoundbank_ascii.h"

namespace Sound {

XACTSoundBank_ASCII::XACTSoundBank_ASCII(Common::SeekableReadStream &xsb) {
	load(xsb);
}

static Common::UString stripZeros(Common::UString str) {
	while ((str.size() > 1) && (*str.begin() == '0'))
		str.erase(str.begin());

	return str;
}

static int64 getNumber(Common::UString str) {
	int64 number = 0;

	Common::parseString(stripZeros(str), number);

	return number;
}

static size_t getAmount(Common::UString str) {
	size_t amount = 0;

	Common::parseString(stripZeros(str), amount);

	return amount;
}

static size_t getAmount(Common::StreamTokenizer &tokenizer, Common::SeekableReadStream &stream) {
	return getAmount(tokenizer.getToken(stream));
}

void XACTSoundBank_ASCII::load(Common::SeekableReadStream &xsb) {
	Common::StreamTokenizer tokenizer(Common::StreamTokenizer::kRuleIgnoreAll);
	tokenizer.addSeparator(' ');
	tokenizer.addChunkEnd('\n');
	tokenizer.addIgnore('\r');
	tokenizer.addQuote('\"');

	_name = tokenizer.getToken(xsb);
	tokenizer.nextChunk(xsb);

	const size_t categoryCount = getAmount(tokenizer, xsb);
	tokenizer.nextChunk(xsb);

	_categories.resize(categoryCount);
	for (Categories::iterator it = _categories.begin(); it != _categories.end(); ++it) {
		it->name = tokenizer.getToken(xsb);
		tokenizer.nextChunk(xsb);
	}

	const size_t soundCount = getAmount(tokenizer, xsb);
	tokenizer.nextChunk(xsb);

	_sounds.resize(soundCount);
	for (Sounds::iterator sound = _sounds.begin(); sound != _sounds.end(); ++sound) {
		std::vector<Common::UString> tokens;
		tokenizer.getTokens(xsb, tokens, 2);
		tokenizer.nextChunk(xsb);

		sound->name = tokens[0];
		sound->category = 0;

		const int64 categoryIndex = getNumber(tokens[1]);
		if ((categoryIndex > 0) && (static_cast<size_t>(categoryIndex) < _categories.size()))
			sound->category = &_categories[categoryIndex];

		_soundMap[sound->name] = &*sound;

		sound->tracks.resize(1);
		Track &track = sound->tracks.back();

		size_t waveCount = 0;

		while (!xsb.eos()) {
			tokenizer.getTokens(xsb, tokens);
			tokenizer.nextChunk(xsb);

			if (tokens.empty())
				break;

			if ((tokens.size() == 1) && (tokens[0].size() == 3)) {
				waveCount = getAmount(tokens[0]);
				break;
			}

			if        (tokens[0] == "3D") {
				// TODO: 3D properties
			} else if (tokens[0] == "PEQ") {
				// TODO: Parametric EQ
			} else if (tokens[0] == "PLAY") {
				bool isComplex = false;

				for (size_t i = 2; i < tokens.size(); i++)
					if (tokens[i] != "0")
						isComplex = true;

				if (isComplex)
					track.events.push_back(Event(kEventTypePlayComplex));
				else
					track.events.push_back(Event(kEventTypePlay));

			} else if (tokens[0] == "LOOP") {
				track.events.push_back(Event(kEventTypeLoop));
			} else if (tokens[0] == "VOLUME") {
				track.events.push_back(Event(kEventTypeVolume));
			} else if (tokens[0] == "PITCH") {
				track.events.push_back(Event(kEventTypePitch));
			} else if (tokens[0] == "LPF") {
				track.events.push_back(Event(kEventTypeLowPass));
			} else if (tokens[0] == "MFLFO") {
				track.events.push_back(Event(kEventTypeLFOMulti));
			} else if (tokens[0] == "MARKER") {
				track.events.push_back(Event(kEventTypeMarker));
			} else if (tokens[0] == "AEG") {
				track.events.push_back(Event(kEventTypeEnvelopeAmplitude));
			} else {
				warning("Unknown event \"%s\"", tokens[0].c_str());
			}
		}

		track.waves.resize(waveCount);
		for (WaveVariations::iterator wave = track.waves.begin(); wave != track.waves.end(); ++wave) {
			tokenizer.getTokens(xsb, tokens, 3);
			tokenizer.nextChunk(xsb);

			wave->name = tokens[1];
			wave->bank = tokens[0];

			wave->index = getNumber(tokens[2]);
		}
	}

	const size_t cueCount = getAmount(tokenizer, xsb);
	tokenizer.nextChunk(xsb);

	_cues.resize(cueCount);
	for (Cues::iterator cue = _cues.begin(); cue != _cues.end(); ++cue) {
		std::vector<Common::UString> tokens;
		tokenizer.getTokens(xsb, tokens, 3);
		tokenizer.nextChunk(xsb);

		cue->name = tokens[0];
		cue->variationSelectMethod = getNumber(tokens[1]);
		const bool hasTransitions = tokens[2] == "1";

		const size_t variationCount = getAmount(tokenizer, xsb);
		tokenizer.nextChunk(xsb);

		cue->variations.resize(variationCount);
		for (CueVariations::iterator variation = cue->variations.begin(); variation != cue->variations.end(); ++variation) {
			tokenizer.getTokens(xsb, tokens, 4);
			tokenizer.nextChunk(xsb);

			variation->soundName = tokens[1];
			variation->soundIndex = getNumber(tokens[1]);

			variation->weightMax = getNumber(tokens[2]);
			variation->weightMin = getNumber(tokens[3]);
		}

		if (!hasTransitions)
			continue;

		const size_t transitionCount = getAmount(tokenizer, xsb);
		tokenizer.nextChunk(xsb);

		cue->transitions.resize(transitionCount);
		for (Transitions::iterator transition = cue->transitions.begin(); transition != cue->transitions.end(); ++transition) {
			tokenizer.getTokens(xsb, tokens, 8);
			tokenizer.nextChunk(xsb);

			transition->from = tokens[0];
			transition->to = tokens[1];

			transition->style = tokens[3];

			transition->parameters[0] = tokens[2];
			transition->parameters[1] = tokens[4];
			transition->parameters[2] = tokens[5];
			transition->parameters[3] = tokens[6];
			transition->parameters[4] = tokens[7];
		}
	}
}

} // End of namespace Sound
