#pragma once
#ifndef TemplateDB
#define TemplateDB

#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unordered_map>
#include "Actor.h"
#include "ComponentManager.h"
#include <memory>

class TemplateDataBase {
public:
    TemplateDataBase(ComponentManager& m);
    std::shared_ptr<Actor> getTemplate(const std::string& s, GameManager& game);
    
private:
    std::shared_ptr<Actor> cloneActor(const std::shared_ptr<Actor>& originalActor);
    std::unordered_map<std::string, std::shared_ptr<Actor>> templates;
    std::shared_ptr<Actor> parseActorFromJson(const rapidjson::Document& json);
    ComponentManager* componentManager;

};
#endif

