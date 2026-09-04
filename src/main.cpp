#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <algorithm>
#include <chrono>
#include "fast_double.h"

#include "cache.h"

using namespace geode::prelude;

int assumption_atoi(const char* p) {
    int x = 0;
    bool neg = false;
    if (*p == '-') {
        neg = true;
        ++p;
    }
    while (*p != '\0') {
        x = (x*10) + (*p - '0');
        ++p;
    }
    if (neg) {
        x = -x;
    }
    return x;
}

float assumption_atof(const char* p) {
    double x;
    return fast_double_parser::parse_number(p, &x) ? x : 0.0f;
}

int assumption_floor(double x) {
  int i = (int)x;
  return i - ( i > x );
}

int assumption_round(double x) {
    return assumption_floor(x + 0.5);
}

template <char C>
size_t find_char(std::string_view view) {
    size_t pos = 0;
    while (view.data()[pos] != C && view.data()[pos] != '\0') {
        ++pos;
    }
    return pos;
}

cocos2d::_ccHSVValue hsvFromString(char const* str_) {
    char* str = const_cast<char*>(str_); // so silly
    cocos2d::_ccHSVValue ret;

    if (*str == 0) 
        return ret;

    char* buf = str;
    do { ++str; } while (*str != 'a');
    *str++ = 0;
    ret.h = assumption_atof(buf);

    buf = str;
    do { ++str; } while (*str != 'a');
    *str++ = 0;
    ret.s = assumption_atof(buf);

    buf = str;
    do { ++str; } while (*str != 'a');
    *str++ = 0;
    ret.v = assumption_atof(buf);

    ret.absoluteSaturation = (*str == '1');
    str += 2;
    ret.absoluteBrightness = (*str == '1');

    return ret;
}

class GameObjectFactory {
protected:
    char const* m_values[109] = { 0 };
    char const* m_startPosValues[23] = { 0 };
    std::unique_ptr<std::string> m_owned;
    bool m_ldm;
public:

    template <typename T = char const*>
    T getValue(int key) {
        auto v = m_values[key];
        return v ? v : ""; 
    }

    template <>
    int getValue<int>(int key) {
        auto v = m_values[key];
        return v ? assumption_atoi(v) : 0;
    }

    template <>
    bool getValue<bool>(int key) {
        auto v = m_values[key];
        return v ? (v[0] == '1') : false;
    }

    template <>
    float getValue<float>(int key) {
        auto v = m_values[key];
        return v ? assumption_atof(v) : 0.0f;
    }

    GameObjectFactory(std::string_view str, bool ldm) {
        m_ldm = ldm;
        m_owned = std::make_unique<std::string>(str);

        std::string_view view = *m_owned;
        std::string::size_type pos = -1;
        bool should_exit = false;

        if (str.size() == 0)
            return;

        do {
            view = view.substr(pos + 1);

            pos = find_char<','>(view);

            const_cast<char*>(view.data())[pos] = 0; // lmao

            bool startpos = false;
            int opcode = 0;
            if (view[0] == 'k' && view[1] == 'A') {
                opcode = assumption_atoi(view.substr(2, pos).data());
                startpos = true;
            } else {
                opcode = assumption_atoi(view.substr(0, pos).data());
                startpos = false;
            }

            view = view.substr(pos + 1);
            pos = find_char<','>(view);

            const_cast<char*>(view.data())[pos] = 0; // lmao

            char const* operand = view.substr(0, pos).data();

            if (startpos)
                m_startPosValues[opcode] = operand;
            else
                m_values[opcode] = operand;
        } while (pos != view.size());
    }

    CCDictionary* startPosString() {
        auto ret = CCDictionary::create();

        for (int i = 0; i < 23; ++i) {
            auto v = m_startPosValues[i];

            if (v) {
                ret->setObject(CCString::create(v), std::string("kA") + std::to_string(i));
            }
        }

        return ret;
    }

    void setupEffectGameObject(EffectGameObject* object) {
        object->m_isTouchTriggered = getValue<bool>(11);
        object->m_isSpawnTriggered = getValue<bool>(62);
        object->m_isMultiTriggered = getValue<bool>(87);
        object->m_itemID = getValue<int>(80);

        switch (object->m_objectID) {
            case 901:
                object->m_easingRate = getValue<float>(85);
                object->m_targetGroupID = getValue<int>(51);
                object->m_moveOffset = ccp(getValue<int>(28), getValue<int>(29));
                object->m_duration = getValue<float>(10);
                object->m_easingType = static_cast<EasingType>(getValue<int>(30));
                object->m_lockToPlayerX = getValue<bool>(58);
                object->m_lockToPlayerY = getValue<bool>(59);
                object->m_useMoveTarget = getValue<bool>(100);
                object->m_centerGroupID = getValue<int>(71);
                object->m_moveTargetMode = static_cast<MoveTargetType>(getValue<int>(101));
                break;
            case 1006:
                object->m_targetGroupID = getValue<int>(51);
                object->m_fadeInDuration = getValue<float>(45);
                object->m_holdDuration = getValue<float>(46);
                object->m_fadeOutDuration = getValue<float>(47);
                object->m_pulseMode = getValue<int>(48);
                object->m_pulseMainOnly = getValue<bool>(65);
                object->m_pulseDetailOnly = getValue<bool>(66);
                object->m_pulseTargetType = getValue<int>(52);
                object->m_pulseExclusive = getValue<int>(86);

                if (object->m_pulseMode) {
                    object->m_hsvValue = hsvFromString(getValue(49));
                    object->m_copyColorID = getValue<int>(50);
                } else {
                    // best-effort rename: no property-tagged 7/8/9 color field remains on
                    // EffectGameObject in current bindings; m_triggerTargetColor is the
                    // same-typed (ccColor3B) field that semantically matches "trigger's own color"
                    object->m_triggerTargetColor = ccc3(getValue<int>(7), getValue<int>(8), getValue<int>(9));
                }

                object->m_duration = getValue<float>(10);
                object->m_opacity = getValue<float>(35);
                break;
            case 1007:
                object->m_targetGroupID = getValue<int>(51);
                object->m_duration = getValue<float>(10);
                object->m_opacity = getValue<float>(35);
                break;
            case 1049:
                object->m_targetGroupID = getValue<int>(51);
                object->m_activateGroup = getValue<bool>(56);
                break;
            case 1268:
                object->m_targetGroupID = getValue<int>(51);
                // m_spawnDelay moved to the SpawnTriggerGameObject subclass in current bindings
                reinterpret_cast<SpawnTriggerGameObject*>(object)->m_spawnDelay = getValue<float>(63);
                object->m_previewDisable = getValue<bool>(102);
                break;
            case 1275:
                object->m_targetGroupID = getValue<int>(51);
                object->m_subtractCount = getValue<bool>(78);
                // TODO: property 79 (pickup mode) has no name in current public Geode bindings.
                // Left unset rather than guessing at an offset/cast that could be wrong.
                // object->m_pickupMode = getValue<int>(79);
                object->m_itemID = getValue<int>(80);
                object->m_activateGroup = getValue<bool>(56);
                break;
            case 1346:
                object->m_targetGroupID = getValue<int>(51);
                object->m_centerGroupID = getValue<int>(71);
                object->m_duration = getValue<float>(10);
                object->m_easingType = static_cast<EasingType>(getValue<int>(30));

                object->m_easingRate = getValue<float>(85);
                object->m_rotationDegrees = getValue<int>(68);
                object->m_times360 = getValue<int>(69);
                object->m_lockObjectRotation = getValue<bool>(70);
                break;
            case 1347:
                object->m_targetGroupID = getValue<int>(51);
                object->m_centerGroupID = getValue<int>(71);
                object->m_duration = getValue<float>(10);
                // m_followMod (CCPoint) split into two separate floats in current bindings
                object->m_followXMod = getValue<float>(72);
                object->m_followYMod = getValue<float>(73);
                // TODO: property 74 is still undocumented in current public Geode bindings
                // (it was already unnamed -- "UndocuementedLevelProperty74" -- in the original)
                // object->UndocuementedLevelProperty74 = getValue<bool>(74);
                break;
            case 1520:
                object->m_duration = getValue<float>(10);
                object->m_shakeStrength = getValue<float>(75);
                object->m_shakeInterval = getValue<float>(84);
                break;
            case 1585:
                object->m_targetGroupID = getValue<int>(51);
                object->m_animationID = getValue<int>(76);
                break;
            case 1595:
                object->m_targetGroupID = getValue<int>(51);
                object->m_activateGroup = getValue<bool>(56);
                object->m_touchHoldMode = getValue<bool>(81);
                object->m_touchToggleMode = static_cast<TouchTriggerType>(getValue<int>(82));
                object->m_isDualMode = getValue<bool>(89);
                break;
            case 1611:
                object->m_itemID = getValue<int>(80);
                object->m_targetGroupID = getValue<int>(51);
                // m_targetCount/m_multiActivate moved to the CountTriggerGameObject subclass
                reinterpret_cast<CountTriggerGameObject*>(object)->m_pickupCount = getValue<int>(77);
                object->m_activateGroup = getValue<bool>(56);
                reinterpret_cast<CountTriggerGameObject*>(object)->m_multiActivate = getValue<bool>(104);
                break;
            case 1616:
                object->m_targetGroupID = getValue<int>(51);
                break;
            case 1587:
            case 1589:
            case 1598:
            case 1614:
                object->m_targetGroupID = getValue<int>(51);
                object->m_subtractCount = getValue<bool>(78);
                // TODO: property 79 (pickup mode) has no name in current public Geode bindings.
                // object->m_pickupMode = getValue<int>(79);
                object->m_itemID = getValue<int>(80);
                object->m_activateGroup = getValue<bool>(56);
                break;
            case 1811:
                object->m_itemID = getValue<int>(80);
                object->m_targetGroupID = getValue<int>(51);
                // m_targetCount/m_comparisonType moved to the CountTriggerGameObject subclass;
                // the field is a plain int now (no more ComparisonType enum on it)
                reinterpret_cast<CountTriggerGameObject*>(object)->m_pickupCount = getValue<int>(77);
                reinterpret_cast<CountTriggerGameObject*>(object)->m_pickupTriggerMode = getValue<int>(88);
                object->m_activateGroup = getValue<bool>(56);
                break;
            case 1812:
                object->m_targetGroupID = getValue<int>(51);
                object->m_activateGroup = getValue<bool>(56);
                break;
            case 1814:
                object->m_followYSpeed = getValue<float>(90);
                object->m_followYDelay = getValue<float>(91);
                object->m_followYOffset = getValue<int>(92);
                object->m_followYMaxSpeed = getValue<float>(105);
                object->m_targetGroupID = getValue<int>(51);
                object->m_duration = getValue<float>(10);
                break;
            case 1815:
                object->m_itemID = getValue<int>(80);
                object->m_itemID2 = getValue<int>(95);
                object->m_targetGroupID = getValue<int>(51);
                object->m_duration = getValue<float>(10);
                object->m_triggerOnExit = getValue<bool>(93);
                object->m_activateGroup = getValue<bool>(56);
                break;
            case 1816:
                object->m_itemID = getValue<int>(80);
                object->m_isDynamicBlock = getValue<bool>(94);
                break;
            case 1817:
                object->m_itemID = getValue<int>(80);
                reinterpret_cast<CountTriggerGameObject*>(object)->m_pickupCount = getValue<int>(77);
                break;

            default:
                break;
        }

        if (object->m_objectID == 105 || object->m_objectID < 31 || (743 < object->m_objectID && object->m_objectID < 901) || object->m_objectID == 915) {
            // Properties 7/8/9 have no property-tagged field anywhere in current bindings
            // (same gap as the pulse-trigger color case above). Best-effort inferred mapping:
            // GJSpriteColor::m_customColor + m_usesCustomBlend is the only field pair with a
            // matching shape (explicit RGB override + a flag to use it) on m_baseColor. Not
            // confirmed by a property comment like the others -- flagging this one for extra
            // scrutiny when testing in-game.
            object->m_baseColor->m_customColor = ccc3(getValue<int>(7), getValue<int>(8), getValue<int>(9));
            object->m_baseColor->m_usesCustomBlend = true;
            object->m_duration = getValue<float>(10);

            object->m_tintGround = getValue<bool>(14);
            object->m_usesPlayerColor1 = getValue<bool>(15);
            object->m_usesPlayerColor2 = getValue<bool>(16);
            object->m_usesBlending = getValue<bool>(17);
            object->m_copyOpacity = getValue<bool>(60);

            int targetCol = getValue<int>(23);
            if (targetCol > 1) {
                object->m_targetColor = targetCol;
            }

            object->m_opacity = getValue<bool>(36) ? getValue<float>(35) : 1.0f;

            object->m_hsvValue = hsvFromString(getValue(49));
            object->m_copyColorID = getValue<int>(50);

            if (object->m_objectID < 31 || object->m_objectID == 105 || object->m_objectID == 900) {
                object->m_usesBlending = false;
                object->m_opacity = 1.0;
            }
        }

        if (object->m_easingRate <= 0.0) {
            object->m_easingRate = 2.0;
        }
    }

    void loadGroups(GameObject* object, std::string_view view) {
        if (view.size() == 0)
            return;

        size_t pos = -1;
        while (pos != view.size()) {
            view = view.substr(pos + 1);
            pos = find_char<'.'>(view);

            const_cast<char*>(view.data())[pos] = '\0';

            object->addToGroup(assumption_atoi(view.data()));
        }
    }

    GameObject* generate() {
        bool highDetail = getValue<bool>(103);

        if (highDetail && m_ldm)
            return nullptr;

        int objectID = getValue<int>(1);
        int idBeforeSwap = objectID;

        switch (objectID) {
            case 221: // color trigger 1
            case 717: // color trigger 2
            case 718: // color trigger 3
            case 743: // color trigger 4
                objectID = 899; // modern color trigger
                break;
            case 1008: // idk
                objectID = 1292;
                break;
            case 104: // line trigger
                objectID = 915; // idk
                break;
            default:
                break;
        }

        auto object = GameObject::createWithKey(objectID);
        if (!object)
            return NULL;


        object->m_objectID = objectID;
        if (object->m_objectID == 1715 || object->m_objectID == 9) {
                object->m_defaultZOrder = 2;
        }

        object->addToGroup(getValue<int>(26));
        object->addToGroup(getValue<int>(33));

        loadGroups(object, getValue(57));

        object->m_zLayer = static_cast<ZLayer>(getValue<int>(24));
        object->m_zOrder = getValue<int>(25);
        // these two live on EffectGameObject in current bindings, not GameObject -- cast needed
        // (the original code wrote these unconditionally on every object regardless of its
        // real dynamic type, same as the other EffectGameObject-only writes below, so keeping
        // that same unconditional pattern here rather than guarding it)
        reinterpret_cast<EffectGameObject*>(object)->m_rotationSpeed = getValue<int>(97);
        reinterpret_cast<EffectGameObject*>(object)->m_disableRotation = getValue<bool>(98);
        object->m_linkedGroup = getValue<int>(108);
        object->m_isHighDetail = getValue<bool>(103);
        object->m_hasGroupParent = getValue<bool>(34);
        object->m_isDontFade = getValue<bool>(64);
        object->m_isDontEnter = getValue<bool>(67);
        object->m_startFlipX = getValue<bool>(4);
        object->m_startFlipY = getValue<bool>(5);

        object->m_editorLayer = getValue<int>(20);
        object->m_editorLayer2 = getValue<int>(61);
        object->m_hasNoGlow = getValue<bool>(96);

        //if (object->animatedCircle) { // too lazy and it doesn't matter
            reinterpret_cast<EffectGameObject*>(object)->m_animationRandomizedStart = getValue<bool>(106);
            reinterpret_cast<EffectGameObject*>(object)->m_animationSpeed = getValue<float>(107);
        //}


        // both of these are guesswork. but they probably work so
        if (getValue<bool>(41)) {
            object->m_baseColor->m_hsv = hsvFromString(getValue(43));
            object->m_baseColor->m_usesHSV = true;
        }
        if (getValue<bool>(42) && object->m_detailColor) {
            object->m_detailColor->m_hsv = hsvFromString(getValue(44));
            object->m_detailColor->m_usesHSV = true;
        }

        float size = getValue<float>(32); // size
        if (size != 0) {
            // optimization mod            
            size = (size == 1.0 ? size : assumption_round(size*100)/100.0);
            // m_scale (single unified field) was split into m_scaleX/m_scaleY
            object->m_scaleX = size;
            object->m_scaleY = size;
            object->setRScale(1.0);

            object->m_isObjectRectDirty = true;
            // m_textureRectDirty no longer exists as a separate flag -- m_isObjectRectDirty
            // above appears to be the sole remaining dirty flag now, so this is dropped
            // rather than guessed at.
        }

        switch (objectID) {
            case 914:
                // updateTextObject moved to the TextGameObject subclass
                reinterpret_cast<TextGameObject*>(object)->updateTextObject(LevelTools::base64DecodeString(getValue(31)), false);
                break;
            case 142:
                // m_secretCoinID lives on EffectGameObject in current bindings
                reinterpret_cast<EffectGameObject*>(object)->m_secretCoinID = getValue<int>(12);
                break;
            case 31:
                reinterpret_cast<StartPosObject*>(object)->m_startSettings = LevelSettingsObject::objectFromDict(startPosString());
                reinterpret_cast<StartPosObject*>(object)->m_startSettings->retain();
                break;
            case 200:
            case 201:
            case 202:
            case 203:
            case 660:
            case 1331:
            case 1334:
            case 47:
            case 111:
            case 13:
                reinterpret_cast<EffectGameObject*>(object)->m_shouldPreview = getValue<bool>(13);
                break;
            case 747:
                reinterpret_cast<TeleportPortalObject*>(object)->m_teleportYOffset = getValue<float>(54);
                reinterpret_cast<TeleportPortalObject*>(object)->m_teleportEase = getValue<bool>(55);
                break;
        }

        reinterpret_cast<EffectGameObject*>(object)->m_isMultiActivate = getValue<bool>(99);
        object->customSetup();

        if (typeinfo_cast<EffectGameObject*>(object)) {
            setupEffectGameObject(reinterpret_cast<EffectGameObject*>(object));
        }

        // addGlow/addColorSprite/setupCustomSprites now require an explicit frame name argument
        // (previously callable with no args); passing "" as the best-effort equivalent of
        // whatever the old implicit default was -- unconfirmed, worth an eye in testing.
        object->addGlow("");
        object->addColorSprite("");
        object->setupCustomSprites("");

        object->setFlipX(object->m_startFlipX);
        object->setFlipY(object->m_startFlipY);

        // m_rotation (single unified field) no longer exists on GameObject -- using a local
        // variable instead, since nothing else in the file reads this field back
        float rotation = getValue<float>(6);

        auto type = static_cast<int>(object->m_objectType);
        if (type == 0 || type == 21 || type == 25) {
            rotation = assumption_floor(rotation / 90.0) * 90.0;
        } else if (type == 36) {
            reinterpret_cast<RingObject*>(object)->m_targetGroupID = getValue<int>(51);
            reinterpret_cast<RingObject*>(object)->m_activateGroup = getValue<bool>(56);
        }

        object->setRotation(rotation);
        object->setStartPos(ccp(getValue<float>(2), getValue<float>(3)+90.0f));
        object->getObjectTextureRect();

        switch (idBeforeSwap) {
            case 743:
                object->m_targetColor = 4;
                break;
            case 718:
                object->m_targetColor = 3;
                break;
            case 717:
                object->m_targetColor = 2;
                break;
            case 221:
                object->m_targetColor = 1;
                break;
            case 104:
                reinterpret_cast<EffectGameObject*>(object)->m_usesBlending = true;
                break;
        }

        int colorChannel;
        switch (getValue<int>(19)) {
            case 0:
                colorChannel = getValue<int>(21);
                if (colorChannel) {
                    if (colorChannel < 0)
                        colorChannel = 1011;

                    object->m_baseColor->m_colorID = colorChannel;
                }

                colorChannel = getValue<int>(22);
                if (colorChannel) {
                    if (object->m_detailColor) {
                        if (colorChannel < 0)
                            colorChannel = 1011;

                        object->m_detailColor->m_colorID = colorChannel;
                    }
                }

                object->saveActiveColors();
                return object;
            case 1:
                colorChannel = 0;
                break;
            case 2:
                colorChannel = 1006;
                break;
            case 3:
                colorChannel = 1;
                break;
            case 4:
                colorChannel = 2;
                break;
            case 5:
                colorChannel = 1007;
                break;
            case 6:
                colorChannel = 3;
                break;
            case 7:
                colorChannel = 4;
                break;
            case 8:
                colorChannel = 1003;
                break;
            default:
                object->saveActiveColors();
                return object;
        }

        auto spritecol = object->m_detailColor;
        if (!spritecol) {
                spritecol = object->m_baseColor;
        }
        spritecol->m_colorID = colorChannel;
        object->saveActiveColors();

        return object;
    }
};

class $modify(MenuLayer) {
    bool init() {
        MenuLayer::init();

        unitObject = new GameObject;
        unitObject->initWithSpriteFrameName("block001_01_001.png");
        unitObject->retain();

        static_cast<FakeSpriteCache*>(CCSpriteFrameCache::sharedSpriteFrameCache())->moveToQuickCache();

        return true;
    }
};

class $modify(test, PlayLayer) {
public:
    void onQuit() {
        PlayLayer::onQuit();
        allocPool.drain();
    }

    void createObjectsFromSetup(gd::string str) {
        std::string real = str;
        if (real.size() > 1) {
            std::string_view view = real;
            intptr_t pos = -1;
            m_levelSettings = nullptr;
            std::vector<GameObject*> coins;

            std::vector<GameObjectFactory> factories;
            int labelObjects = 0;

            while (pos != view.size()) {
                view = view.substr(pos + 1);
                pos = find_char<';'>(view);

                auto object_string = view.substr(0, pos);

                if (!m_levelSettings) {
                    m_levelSettings = LevelSettingsObject::objectFromString(std::string(object_string));
                    m_levelSettings->retain();
                    m_levelSettings->m_level = m_level;
                    m_levelSettings->m_effectManager->updateColors(m_player1->m_playerColor1, m_player1->m_playerColor2);
                    GameManager::sharedState()->loadFont(m_levelSettings->m_fontIndex);

                    continue;
                }
                
                factories.push_back(std::move(GameObjectFactory(object_string, m_level->m_lowDetailModeToggled)));

            }

            allocPool.drain();
            allocPool.alloc(factories.size());

            for (auto& factory : factories) {
                GameObject* object = factory.generate();

                // GJLevelType has no "Local" value in current bindings; "Editor" is the
                // closest semantic match (a level currently being edited/tested, as opposed
                // to Main/Saved/SearchResult) -- best-effort, not confirmed against source.
                if (!object || (m_level->m_levelType != GJLevelType::Editor && object->m_objectType == GameObjectType::SecretCoin))
                    continue;

                if (object->m_objectType == GameObjectType::UserCoin && coins.size() < 3)
                    coins.push_back(object);

                addObject(object);
            }

            if (!coins.empty()) {
                std::sort(coins.begin(), coins.end(), [](GameObject* o1, GameObject* o2) {
                    return o1->getPositionX() < o2->getPositionX();
                });

                for (int i = 0; i < coins.size(); ++i) {
                    reinterpret_cast<EffectGameObject*>(coins[i])->m_secretCoinID = i + 1;
                    // setupCoinArt() has no equivalent anywhere in current bindings (genuine
                    // gap, not a rename). Left out rather than guessed -- likely means coin
                    // sprites won't get their per-coin visual variant applied.
                    // coins[i]->setupCoinArt();
                }
            }
        }

        float screenEnd = CCDirector::sharedDirector()->getScreenRight() + 300;
        // m_realLevelLength no longer exists as a separate field -- only m_levelLength remains,
        // so reading from it too (rather than a distinct "real" value) is the closest available
        // equivalent; best-effort, not confirmed.
        m_levelLength = fmax(screenEnd, m_levelLength + 340);

        m_endPortal = EndPortalObject::create();
        m_endPortal->setStartPos(ccp(m_levelLength, 225.0));
        m_endPortal->m_defaultZOrder = 11;
        addToSection(m_endPortal);
        m_objects->addObject(m_endPortal);

        m_endPortal->updateColors(m_player1->m_playerColor1);
        m_endPortal->setVisible(false);

        // m_spawnObjects2 (a CCArray with addObject) is now called m_spawnObjectsArray
        m_spawnObjectsArray->addObject(m_endPortal);
        // calculateSpawnXPos() has no equivalent in current bindings; EndPortalObject now
        // exposes getSpawnPos() (a plain getter) instead of an explicit "calculate and cache"
        // call, which suggests the value may just be computed on demand now rather than
        // needing to be precomputed here. Dropped rather than guessed at.
        // m_endPortal->calculateSpawnXPos();

        // The raw SpeedObject type and manual sort are gone; Geode now exposes GD's own
        // sorting logic directly, so using that instead of guessing at the removed type's
        // layout.
        LevelTools::sortSpeedObjects(m_speedObjects, this);
    }
};
