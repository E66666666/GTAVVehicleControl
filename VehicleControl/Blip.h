#pragma once
#include <string>
#include "inc/natives.h"
#include "inc/enums.h"
#include "script.h"


class Blip_ {
public:
    Blip_(Entity entity, eBlipSprite blip, std::string name, eBlipColor color, bool showHeading)
        : mHandle(HUD::ADD_BLIP_FOR_ENTITY(entity))
        , mEntity(entity)
        , mName(name) {
        SetSprite(blip);
        SetName(name);
        SetColor(color);
        ShowHeading(showHeading);
    }

    ~Blip_() {
        /*if (Unloading())
            return;

        Delete();*/
    }

    Vector3 GetPosition() const {
        return HUD::GET_BLIP_INFO_ID_COORD(mHandle);
    }

    void SetPosition(Vector3 coord) {
        HUD::SET_BLIP_COORDS(mHandle, coord.x, coord.y, coord.z);
    }

    eBlipColor GetColor() const {
        return (eBlipColor)HUD::GET_BLIP_COLOUR(mHandle);
    }

    void SetColor(eBlipColor color) {
        HUD::SET_BLIP_COLOUR(mHandle, color);
    }

    int GetAlpha() const {
        return HUD::GET_BLIP_ALPHA(mHandle);
    }

    void SetAlpha(int alpha) {
        HUD::SET_BLIP_ALPHA(mHandle, alpha);
    }

    void SetRotation(int rotation) {
        HUD::SET_BLIP_ROTATION(mHandle, rotation);
    }

    void SetScale(float scale) {
        HUD::SET_BLIP_SCALE(mHandle, scale);
    }

    eBlipSprite GetSprite() const {
        return (eBlipSprite)HUD::GET_BLIP_SPRITE(mHandle);
    }

    void SetSprite(eBlipSprite sprite) {
        HUD::SET_BLIP_SPRITE(mHandle, sprite);
    }

    Entity GetEntity() const {
        return mEntity;// HUD::GET_BLIP_INFO_ID_ENTITY_INDEX(mHandle);
    }

    std::string GetName() {
        return mName;
    }

    void SetName(std::string name) {
        HUD::BEGIN_TEXT_COMMAND_SET_BLIP_NAME("STRING");
        HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME((char*)name.c_str());
        HUD::END_TEXT_COMMAND_SET_BLIP_NAME(mHandle);
    }

    void Delete() {
        HUD::SET_BLIP_ALPHA(mHandle, 0);
        HUD::REMOVE_BLIP(&mHandle);
    }

    bool Exists() const {
        return HUD::DOES_BLIP_EXIST(mHandle);
    }

    Blip GetHandle() const {
        return mHandle;
    }

    void ShowHeading(bool b) {
        HUD::SHOW_HEADING_INDICATOR_ON_BLIP(mHandle, b);
    }

private:
    int mHandle;
    Entity mEntity;
    std::string mName;
};
