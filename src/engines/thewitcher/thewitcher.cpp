/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names can be
 * found in the AUTHORS file distributed with this source
 * distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * The Infinity, Aurora, Odyssey, Eclipse and Lycium engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 */

/** @file engines/thewitcher/thewitcher.cpp
 *  Engine class handling The Witcher
 */

#include "common/util.h"
#include "common/filelist.h"
#include "common/filepath.h"
#include "common/stream.h"
#include "common/configman.h"

#include "aurora/resman.h"
#include "aurora/error.h"

#include "graphics/cursorman.h"
#include "graphics/cameraman.h"

#include "graphics/aurora/sceneman.h"
#include "graphics/aurora/model_witcher.h"

#include "sound/sound.h"

#include "events/events.h"

#include "engines/aurora/util.h"
#include "engines/aurora/resources.h"

#include "engines/thewitcher/thewitcher.h"
#include "engines/thewitcher/model.h"

namespace Engines {

namespace TheWitcher {

const TheWitcherEngineProbe kTheWitcherEngineProbe;

const Common::UString TheWitcherEngineProbe::kGameName = "The Witcher";

TheWitcherEngineProbe::TheWitcherEngineProbe() {
}

TheWitcherEngineProbe::~TheWitcherEngineProbe() {
}

Aurora::GameID TheWitcherEngineProbe::getGameID() const {
	return Aurora::kGameIDTheWitcher;
}

const Common::UString &TheWitcherEngineProbe::getGameName() const {
	return kGameName;
}

bool TheWitcherEngineProbe::probe(const Common::UString &directory, const Common::FileList &rootFiles) const {
	// There should be a system directory
	Common::UString systemDir = Common::FilePath::findSubDirectory(directory, "system", true);
	if (systemDir.empty())
		return false;

	// The system directory has to be readable
	Common::FileList systemFiles;
	if (!systemFiles.addDirectory(systemDir))
		return false;

	// If either witcher.ini or witcher.exe exists, this should be a valid path
	return systemFiles.contains(".*/witcher.(exe|ini)", true);
}

bool TheWitcherEngineProbe::probe(Common::SeekableReadStream &stream) const {
	return false;
}

Engines::Engine *TheWitcherEngineProbe::createEngine() const {
	return new TheWitcherEngine;
}


TheWitcherEngine::TheWitcherEngine() {
}

TheWitcherEngine::~TheWitcherEngine() {
}

void TheWitcherEngine::run(const Common::UString &target) {
	_baseDirectory = target;

	init();
	initCursors();

	if (EventMan.quitRequested())
		return;

	status("Successfully initialized the engine");

	CursorMan.hideCursor();

	playVideo("publisher");
	playVideo("developer");
	playVideo("engine");
	playVideo("intro");
	playVideo("title");

	CursorMan.set();
	CursorMan.showCursor();

	if (EventMan.quitRequested())
		return;

	playSound("menu", Sound::kSoundTypeMusic, true);

	CameraMan.setPosition(0.0, 0.5, 0.0);

	Graphics::Aurora::Model_Witcher *model = createWorldModel("an_cat");

	model->rotate(Common::deg2rad(225), 0.0, 0.0, 1.0);
	model->setPosition(0.0, 0.0, -2.0);
	model->setVisible(true);

	EventMan.enableKeyRepeat();

	while (!EventMan.quitRequested()) {
		Events::Event event;

		while (EventMan.pollEvent(event)) {
			if        (event.type == Events::kEventMouseMove) {
				if (event.motion.state & SDL_BUTTON(2)) {
					CameraMan.pitch(-Common::deg2rad(0.5) * event.motion.yrel);
					CameraMan.rotate(-Common::deg2rad(0.5) * event.motion.xrel, 0.0, 1.0, 0.0);
				}
			} else if (event.type == Events::kEventKeyDown) {
				float speed = (event.key.keysym.mod & KMOD_SHIFT) ? 1.0 : 0.5;

				if      (event.key.keysym.sym == SDLK_UP)
					CameraMan.moveRelative(0.0, 0.0, -speed);
				else if (event.key.keysym.sym == SDLK_DOWN)
					CameraMan.moveRelative(0.0, 0.0,  speed);
				else if (event.key.keysym.sym == SDLK_RIGHT)
					CameraMan.rotate(Common::deg2rad(-5), 0.0, 1.0, 0.0);
				else if (event.key.keysym.sym == SDLK_LEFT)
					CameraMan.rotate(Common::deg2rad( 5), 0.0, 1.0, 0.0);
				else if (event.key.keysym.scancode == SDL_SCANCODE_W)
					CameraMan.moveRelative(0.0, 0.0, -speed);
				else if (event.key.keysym.scancode == SDL_SCANCODE_S)
					CameraMan.moveRelative(0.0, 0.0,  speed);
				else if (event.key.keysym.scancode == SDL_SCANCODE_D)
					CameraMan.rotate(Common::deg2rad(-5), 0.0, 1.0, 0.0);
				else if (event.key.keysym.scancode == SDL_SCANCODE_A)
					CameraMan.rotate(Common::deg2rad( 5), 0.0, 1.0, 0.0);
				else if (event.key.keysym.scancode == SDL_SCANCODE_E)
					CameraMan.moveRelative( speed, 0.0, 0.0);
				else if (event.key.keysym.scancode == SDL_SCANCODE_Q)
					CameraMan.moveRelative(-speed, 0.0, 0.0);
				else if (event.key.keysym.sym == SDLK_INSERT)
					CameraMan.moveRelative(0.0,  speed, 0.0);
				else if (event.key.keysym.sym == SDLK_DELETE)
					CameraMan.moveRelative(0.0, -speed, 0.0);
				else if (event.key.keysym.sym == SDLK_PAGEUP)
					CameraMan.pitch(Common::deg2rad(5));
				else if (event.key.keysym.sym == SDLK_PAGEDOWN)
					CameraMan.pitch(Common::deg2rad(-5));
				else if (event.key.keysym.sym == SDLK_END) {
					float x, y, z;
					CameraMan.getDirection(x,   y, z);
					CameraMan.setDirection(x, 0.0, z);
				}
			}
		}

		EventMan.delay(10);
	}

	destroyModel(model);
}

void TheWitcherEngine::init() {
	status("Setting base directory");
	ResMan.registerDataBaseDir(_baseDirectory);

	status("Adding extra archive directories");
	ResMan.addArchiveDir(Aurora::kArchiveEXE, "system");
	ResMan.addArchiveDir(Aurora::kArchiveKEY, "data");
	ResMan.addArchiveDir(Aurora::kArchiveBIF, "data");
	ResMan.addArchiveDir(Aurora::kArchiveBIF, "data/voices");
	ResMan.addArchiveDir(Aurora::kArchiveERF, "data/modules/!final");

	status("Loading main KEY");
	indexMandatoryArchive(Aurora::kArchiveKEY, "main.key", 1);

	status("Loading the localized base KEY");
	indexMandatoryArchive(Aurora::kArchiveKEY, "localized.key", 10);

	status("Loading the English language KEYs");
	indexMandatoryArchive(Aurora::kArchiveKEY, "lang_3.key", 20);
	indexMandatoryArchive(Aurora::kArchiveKEY, "M1_3.key"  , 21);
	indexMandatoryArchive(Aurora::kArchiveKEY, "M2_3.key"  , 22);

	status("Indexing extra resources");
	indexOptionalDirectory("data/movies"   , 0, -1, 30);
	indexOptionalDirectory("data/music"    , 0, -1, 31);
	indexOptionalDirectory("data/sounds"   , 0, -1, 32);
	indexOptionalDirectory("data/cutscenes", 0, -1, 33);
	indexOptionalDirectory("data/dialogues", 0, -1, 34);
	indexOptionalDirectory("data/fx"       , 0, -1, 35);
	indexOptionalDirectory("data/meshes"   , 0, -1, 36);
	indexOptionalDirectory("data/quests"   , 0, -1, 37);
	indexOptionalDirectory("data/scripts"  , 0, -1, 38);
	indexOptionalDirectory("data/templates", 0, -1, 39);
	indexOptionalDirectory("data/textures" , 0, -1, 40);

	indexOptionalDirectory("data", ".*\\.bik", 0, 41);

	status("Indexing Windows-specific resources");
	indexMandatoryArchive(Aurora::kArchiveEXE, "witcher.exe", 42);

	status("Indexing override files");
	indexOptionalDirectory("data/override", 0, 0, 50);

	SceneMan.registerModelType(Graphics::Aurora::kModelTypeTheWitcher);
}

void TheWitcherEngine::initCursors() {
	CursorMan.add("cursor0" , "default"  , "up"  );
	CursorMan.add("cursor1" , "default"  , "down");

	CursorMan.setDefault("default", "up");
}

} // End of namespace TheWitcher

} // End of namespace Engines
