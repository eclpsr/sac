/*
    This file is part of sac.

    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer

    Heriswap is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    Heriswap is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Heriswap.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "LocalizeAPILinuxImpl.h"
#ifndef SAC_EMSCRIPTEN
	#if defined(SAC_WINDOWS) || defined(SAC_DARWIN)
	//TODO
	#else
	#include <libintl.h>
	#include <locale.h>
	#endif
#endif


#if defined(SAC_DARWIN) || defined(SAC_WINDOWS)


int LocalizeAPILinuxImpl::init(const std::string & lang) {
	return 0;
}

#else

#include <tinyxml2.h>
#include "base/Log.h"
#include "api/AssetAPI.h"

int LocalizeAPILinuxImpl::init(AssetAPI* assetAPI, const std::string & lang) {

    std::string filename = "values";

    if (lang != "en") {
        filename += "-";
        filename += lang.c_str();
    }
    filename += "/strings.xml";
    LOGE("Found language:" << lang << " -> " << filename)

    //first, clean the map
    _idToMessage.clear();

    FileBuffer fb = assetAPI->loadAsset(filename);

    tinyxml2::XMLDocument doc;
    doc.Parse((const char*)fb.data);

    tinyxml2::XMLHandle hDoc(&doc);
    tinyxml2::XMLElement * pElem;

    pElem = hDoc.FirstChildElement().ToElement();
    tinyxml2::XMLHandle hRoot(pElem);

    for (pElem = hRoot.FirstChildElement().ToElement(); pElem;
    pElem = pElem->NextSiblingElement()) {

        std::string s = pElem->GetText();

        while (s.find("\\n") != std::string::npos) {
            s.replace(s.find("\\n"), 2, "\n");
        }
        if (s.find("\"") == 0) {
            s = s.substr(1, s.length() - 2);
        }
        _idToMessage[pElem->Attribute("name")] = s;
        LOGV(1, "'" << _idToMessage[pElem->Attribute("name")] << "' = '" << s << "'")
    }
    LOGI("Localize strings count: " << _idToMessage.size())

    return 0;
}
#endif

std::string LocalizeAPILinuxImpl::text(const std::string& s, const std::string&) {
    #ifdef SAC_DEBUG
    if (_idToMessage.find(s) != _idToMessage.end()) {
        LOG_EVERY_N(LOGE, 100, "'" << s << "' is not a valid localize ID");
    }
    #endif

    return _idToMessage[s];
}
