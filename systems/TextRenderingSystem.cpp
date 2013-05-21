#include "TextRenderingSystem.h"
#include <ctype.h>
#include <sstream>
#include <iomanip>

#include <glm/glm.hpp>
#include "base/EntityManager.h"

#include "AnchorSystem.h"
#include "TransformationSystem.h"
#include "RenderingSystem.h"

#include "util/MurmurHash.h"

struct CacheKey {
    Color color;
    float charHeight;
    float positioning;
    bool show;
    int flags;
    struct {
        bool show;
        float speed;
        float dt;
    } caret;
    unsigned cameraBitMask;
    char textFont[2048];

    unsigned populate(TextRenderingComponent* tc) {
        color = tc->color;
        charHeight = tc->charHeight;
        positioning = tc->positioning;
        // TODO
        show = tc->show;
        flags = tc->flags;
        memcpy(&caret, &tc->caret, sizeof(caret));
        cameraBitMask = tc->cameraBitMask;
        int length = tc->text.length();
        memcpy(textFont, tc->text.c_str(), length);
        memcpy(&textFont[length], tc->fontName.c_str(), tc->fontName.length());
        length += tc->fontName.length();
        return sizeof(CacheKey) - 2048 + length;
    }
};

const float TextRenderingComponent::LEFT = 0.0f;
const float TextRenderingComponent::CENTER = 0.5f;
const float TextRenderingComponent::RIGHT = 1.0f;

const char InlineImageDelimiter[] = {(char)0xC3, (char)0x97};

// Utility functions
static Entity createRenderingEntity();
static void parseInlineImageString(const std::string& s, std::string* image, float* w, float* h);
static float computePartialStringWidth(TextRenderingComponent* trc, size_t from, size_t to, float charHeight, const TextRenderingSystem::FontDesc& fontDesc);
static float computeStringWidth(TextRenderingComponent* trc, float charHeight, const TextRenderingSystem::FontDesc& fontDesc);
static float computeStartX(TextRenderingComponent* trc, float charHeight, const TextRenderingSystem::FontDesc& fontDesc);

// System implementation
INSTANCE_IMPL(TextRenderingSystem);

TextRenderingSystem::TextRenderingSystem() : ComponentSystemImpl<TextRenderingComponent>("TextRendering") {
    TextRenderingComponent tc;
    componentSerializer.add(new StringProperty("text", OFFSET(text, tc)));
    componentSerializer.add(new StringProperty("font_name", OFFSET(fontName, tc)));
    componentSerializer.add(new Property<Color>("color", OFFSET(color, tc)));
    componentSerializer.add(new Property<float>("char_height", OFFSET(charHeight, tc), 0.001));
    componentSerializer.add(new Property<float>("positioning", OFFSET(positioning, tc), 0.001));
    componentSerializer.add(new Property<bool>("show", OFFSET(show, tc)));
}

void TextRenderingSystem::DoUpdate(float dt) {
    if (!components.empty() && fontRegistry.empty()) {
        LOGW("Trying to use TextRendering, with no font defined");
        return;
    }

    CacheKey key;
    unsigned letterCount = 0;

    LOGT_EVERY_N(6000, "TODO: textrendering cache");

    FOR_EACH_ENTITY_COMPONENT(TextRendering, entity, trc)
        // compute cache entry
        if (0 && trc->blink.onDuration == 0) {
            unsigned keySize = key.populate(trc);
            unsigned hash = MurmurHash::compute(&key, keySize, 0x12345678);
            std::map<Entity, unsigned int>::iterator c = cache.find(entity);
            if (c != cache.end()) {
                if (hash == (*c).second)
                    continue;
            }
            cache[entity] = hash;
        }
        LOGV(3, "Text: '" << trc->text << "'");

		// early quit if hidden
		if (!trc->show) {
			continue;
		}

        // append a caret if needed
		bool caretInserted = false;
		if (trc->caret.speed > 0) {
			trc->caret.dt += dt;
			if (trc->caret.dt > trc->caret.speed) {
				trc->caret.dt = 0;
				trc->caret.show = !trc->caret.show;
			}
            if (trc->caret.show) {
			    caretInserted = true;
			    trc->text.push_back('_');
            }
		}
        // text blinking
        if (trc->blink.onDuration > 0) {
            if (trc->blink.accum >= 0) {
                trc->blink.accum = glm::min(trc->blink.accum + dt, trc->blink.onDuration);
                trc->show = true;
                if (trc->blink.accum == trc->blink.onDuration) {
                    trc->blink.accum = -trc->blink.offDuration;
                }
            } else {
                trc->blink.accum += dt;
                trc->blink.accum = glm::min(trc->blink.accum + dt, 0.0f);
                trc->show = false;
                if (trc->blink.accum >= 0)
                    trc->blink.accum = 0;
            }
        }

        // Lookup font description
        std::map<std::string, FontDesc>::const_iterator fontIt = fontRegistry.find(trc->fontName);
        if (fontIt == fontRegistry.end()) {
            LOGE("TextRendering component uses undefined font: '" << trc->fontName << "'");
            continue;
        }

        // Cache various attributes
        const FontDesc& fontDesc = fontIt->second;
        const TransformationComponent* trans = TRANSFORM(entity);
		const unsigned int length = trc->text.length();

        // Determine font size (character height)
		float charHeight = trc->charHeight;
		if (trc->flags & TextRenderingComponent::AdjustHeightToFillWidthBit) {
			const float targetWidth = trans->size.x;
			charHeight = targetWidth / computeStringWidth(trc, 1, fontDesc);
            // Limit to maxCharHeight if defined
			if (trc->maxCharHeight > 0 ) {
				charHeight = glm::min(trc->maxCharHeight, charHeight);
			}
		}

        // Variables
		const float startX = (trc->flags & TextRenderingComponent::MultiLineBit) ?
			(trans->size.x * -0.5) : computeStartX(trc, charHeight, fontDesc);
		float x = startX, y = 0;
		bool newWord = true;
        uint16_t unicode = 0; // limited to short

        // Setup rendering for each individual letter
		for(unsigned int i=0; i<length; i++) {
            // If it's a multiline text, we must compute words/lines boundaries
			if (trc->flags & TextRenderingComponent::MultiLineBit) {
				size_t wordEnd = trc->text.find_first_of(" ,:.", i);
				size_t lineEnd = trc->text.find_first_of("\n", i);
				bool newLine = false;
				if (wordEnd == i) {
                    // next letter will be the start of a new word
					newWord = true;
				} else if (lineEnd == i) {
                    // next letter will be the start of a new line
					newLine = true;
				} else if (newWord) {
					if (wordEnd == std::string::npos) {
						wordEnd = trc->text.length();
					}
					// compute length of next word
					const float w = computePartialStringWidth(trc, i, wordEnd - 1, charHeight, fontDesc);
                    // If it doesn't fit on current line -> start new line
					if (x + w >= trans->size.x * 0.5) {
						newLine = true;
					}
					newWord = false;
				}
                // Begin new line if requested
				if (newLine) {
					y -= 1.2 * charHeight;
					x = startX;
					if (lineEnd == i) {
					  continue;
					}
				}
			}

			std::stringstream a;
			unsigned char letter = (unsigned char)trc->text[i];
            int skip = -1;

            if (letter >= 0xC2) {
                LOGW_IF(unicode != 0, "3+ bytes support for UTF8 not complete");
                unicode = (letter - 0xC2) * 0x40;
                continue;
            } else {
                unicode += letter;
            }

            // Add rendering entity if needed
            if (letterCount >= renderingEntitiesPool.size()) {
                renderingEntitiesPool.push_back(createRenderingEntity());
            }
            const Entity e = renderingEntitiesPool[letterCount];
            RenderingComponent* rc = RENDERING(e);
            TransformationComponent* tc = TRANSFORM(e);
            AnchorComponent* ac = ANCHOR(e);
            ac->parent = entity;
            ac->z = 0.001; // put text in front
            ac->position = trans->position;
            rc->show = trc->show;
            rc->cameraBitMask = trc->cameraBitMask;

            // At this point, we have the proper unicodeId to display,
            // except if it's an image delimiter
            bool inlineImage = false;
            if (unicode == 0x00D7) {
                size_t next = trc->text.find(InlineImageDelimiter, i+1, 2);
                LOGV(3, "Inline image '" << trc->text.substr(i, next - i + 1) << "'");
                LOGE_IF(next == std::string::npos, "Malformed string, cannot find inline image delimiter: '" << trc->text << "'");
                std::string texture;
                parseInlineImageString(
                    trc->text.substr(i+1, next - 1 - (i+1) + 1), &texture, &tc->size.x, &tc->size.y);
                rc->texture = theRenderingSystem.loadTextureFile(texture);
                rc->color = Color();
                tc->size.x *= charHeight;
                tc->size.y *= charHeight;
                inlineImage = true;
                // skip inline image letters
                skip = next + 1;
            } else {
                if (unicode > fontDesc.highestUnicode) {
                    LOGW("Missing unicode char: " << unicode << "(highest one: " << fontDesc.highestUnicode << ')');
                    unicode = 0;
                }
                const CharInfo& info = fontDesc.entries[unicode];
                tc->size = glm::vec2(charHeight * info.h2wRatio, charHeight);
                // if letter is space, hide it
                if (unicode == 0x20) {
                    rc->show = false;
                } else {
                    rc->texture = info.texture;
                    rc->color = trc->color;
                }
            }
            // Advance position
            letterCount++;
			x += tc->size.x * 0.5;
			ac->position.x = x;
            ac->position.y = y + (inlineImage ? tc->size.x * 0.25 : 0);
			x += tc->size.x * 0.5;
            unicode = 0;

            // Special case for numbers rendering, add semi-space to group (e.g: X XXX XXX)
 			if (trc->flags & TextRenderingComponent::IsANumberBit && ((length - i - 1) % 3) == 0) {
				x += fontDesc.entries[(unsigned)'a'].h2wRatio * charHeight * 0.75;
			}

            // Fastforward to skip some chars (e.g: inline image description)
            if (skip >= 0) {
                i = skip;
            }
		}

        // if we appended a caret, remove it
		if (caretInserted) {
			trc->text.resize(trc->text.length() - 1);
		}
	}
    if (renderingEntitiesPool.size() > letterCount*2) {
        for (unsigned i=letterCount*2; i<renderingEntitiesPool.size(); i++) {
            theEntityManager.DeleteEntity(renderingEntitiesPool[i]);
        }
        renderingEntitiesPool.resize(letterCount*2);
    }
    for (unsigned i=letterCount; i<renderingEntitiesPool.size(); i++) {
        const Entity e = renderingEntitiesPool[i];
        ANCHOR(e)->parent = 0;
        RENDERING(e)->show = false;
    }
}

void TextRenderingSystem::registerFont(const std::string& fontName, const std::map<uint32_t, float>& charH2Wratio) {
    uint32_t highestUnicode = 0;
    std::for_each(charH2Wratio.begin(), charH2Wratio.end(),
        [&highestUnicode] (std::pair<uint32_t, float> a) { if (a.first > highestUnicode) highestUnicode = a.first; });

    std::string invalidChar = "00_" + fontName;
    TextureRef invalidCharTexture = theRenderingSystem.loadTextureFile(invalidChar);
    float invalidRatio = charH2Wratio.find(0)->second;
    FontDesc font;
    font.highestUnicode = highestUnicode;
    font.entries = new CharInfo[highestUnicode + 1];
    // init
    for (unsigned i=0; i<=highestUnicode; i++) {
        font.entries[i].texture = invalidCharTexture;
        font.entries[i].h2wRatio = invalidRatio;
    }

    for (std::map<uint32_t, float>::const_iterator it=charH2Wratio.begin(); it!=charH2Wratio.end(); ++it) {
        CharInfo& info = font.entries[it->first];
        info.h2wRatio = it->second;
        std::stringstream ss;
        ss.fill('0');
        ss << std::hex << std::setw(2) << it->first << '_' << fontName;
        info.texture = theRenderingSystem.loadTextureFile(ss.str());
        if (info.texture == InvalidTextureRef) {
            LOGW("Font '" << fontName << "' uses unknown texture: '" << ss.str() << "'");
        }
    }
    font.entries[(unsigned)' '].h2wRatio = font.entries[(unsigned)'r'].h2wRatio;
    fontRegistry[fontName] = font;
}

float TextRenderingSystem::computeTextRenderingComponentWidth(TextRenderingComponent* trc) const {
    // Lookup font description
    std::map<std::string, FontDesc>::const_iterator fontIt = fontRegistry.find(trc->fontName);
    if (fontIt == fontRegistry.end()) {
        LOGE("TextRendering component uses undefined font: '" << trc->fontName << "'");
        return 0;
    }
    return computeStringWidth(trc, trc->charHeight, fontIt->second);
}

#if SAC_INGAME_EDITORS
void TextRenderingSystem::addEntityPropertiesToBar(Entity entity, TwBar* bar) {
    TextRenderingComponent* tc = Get(entity, false);
    if (!tc) return;
    TwAddVarRW(bar, "text", TW_TYPE_STDSTRING, &tc->text, "group=TextRendering");
    TwAddVarRW(bar, "text_color", TW_TYPE_COLOR4F, &tc->color, "group=TextRendering");
    TwAddVarRW(bar, "text_show", TW_TYPE_BOOLCPP, &tc->show, "group=TextRendering");
    TwAddVarRW(bar, "text_height", TW_TYPE_FLOAT, &tc->charHeight, "group=TextRendering step=0,01");
    TwAddVarRW(bar, "text_fontName", TW_TYPE_STDSTRING, &tc->fontName, "group=TextRendering");
    TwAddVarRW(bar, "positioning", TW_TYPE_FLOAT, &tc->positioning, "group=TextRendering step=0,01");
    TwAddVarRW(bar, "text_flags", TW_TYPE_INT32, &tc->flags, "group=TextRendering");
    TwAddVarRW(bar, "blink_offDuration", TW_TYPE_FLOAT, &tc->blink.offDuration, "group=Blinking step=0,01");
    TwAddVarRW(bar, "blink_onDuration", TW_TYPE_FLOAT, &tc->blink.onDuration, "group=Blinking step=0,01");
    TwAddVarRW(bar, "blink_accum", TW_TYPE_FLOAT, &tc->blink.accum, "group=Blinking step=0,01");
    TwAddVarRW(bar, "caret_show", TW_TYPE_BOOLCPP, &tc->caret.show, "group=Caret");
    TwAddVarRW(bar, "caret_speed", TW_TYPE_FLOAT, &tc->caret.speed, "group=Caret");
    TwAddVarRW(bar, "caret_dt", TW_TYPE_FLOAT, &tc->caret.dt, "group=Caret");
    std::stringstream groups;
    groups << TwGetBarName(bar) << '/' << "Blinking group=TextRendering\t\n" << TwGetBarName(bar) << '/' << "Caret group=TextRendering";
    TwDefine(groups.str().c_str());
}
#endif

static Entity createRenderingEntity() {
    Entity e = theEntityManager.CreateEntity("__text_letter");
    ADD_COMPONENT(e, Transformation);
    ADD_COMPONENT(e, Rendering);
    ADD_COMPONENT(e, Anchor);
    return e;
}

static void parseInlineImageString(const std::string& s, std::string* image, float* w, float* h) {
    int idx0 = s.find(',');
    int idx1 = s.find(',', idx0 + 1);
    if (image)
        *image = s.substr(0, idx0);
    if (w)
        *w = atof(s.substr(idx0 + 1, idx1 - idx0).c_str());
    if (h)
        *h = atof(s.substr(idx1 + 1, s.length() - idx1).c_str());
}

static float computePartialStringWidth(TextRenderingComponent* trc, size_t from, size_t toInc, float charHeight, const TextRenderingSystem::FontDesc& fontDesc) {
    float width = 0;
    // If it's a number, pre-add grouping spacing
    if (trc->flags & TextRenderingComponent::IsANumberBit) {
        float spaceW = fontDesc.entries[(unsigned)'a'].h2wRatio * charHeight * 0.75;
        width += ((int) (from - toInc) / 3) * spaceW;
    }
    uint16_t unicode = 0;
    for (unsigned int i=from; i<toInc; i++) {
        unsigned char letter = (unsigned char)trc->text[i];
        if (letter >= 0xC2) {
            LOGW_IF(unicode != 0, "3+ bytes support for UTF8 not complete");
            unicode = (letter - 0xC2) * 0x40;
            continue;
        }
        unicode += letter;
        if (unicode == 0x00D7) {
            size_t next = trc->text.find(InlineImageDelimiter, i+1, 2);
            LOGE_IF(next == std::string::npos, "Malformed string, cannot find inline image delimiter: '" << trc->text << "'");
            float w = 0;
            parseInlineImageString(
                trc->text.substr(i+1, next - 1 - (i+1) + 1), 0, &w, 0);
            width += w * charHeight;
            i = next + 1;
        } else if (unicode == '\n') {

        } else {
            width += fontDesc.entries[unicode].h2wRatio * charHeight;
        }
        unicode = 0;
    }
    return width;
}

static float computeStringWidth(TextRenderingComponent* trc, float charHeight, const TextRenderingSystem::FontDesc& fontDesc) {
    float width = computePartialStringWidth(trc, 0, trc->text.length(), charHeight, fontDesc);
    LOGV(2, "String width: '" << trc->text << "' = " << width);
    return width;
}

static float computeStartX(TextRenderingComponent* trc, float charHeight, const TextRenderingSystem::FontDesc& fontDesc) {
    const float result = -computeStringWidth(trc, charHeight, fontDesc) * trc->positioning;
    return result;
}
