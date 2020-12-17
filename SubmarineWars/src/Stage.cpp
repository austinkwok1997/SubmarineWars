//
//  Stage.cpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-10-12.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#include "Stage.hpp"
#include "Foundations/JSON.hpp"
#include "Foundations/Debug.hpp"
#include "ProjectPath.hpp"
#include <fstream>
#include <sstream>

using JSON = nlohmann::json;

///
/// Load a stage
///
/// @param number The stage number
///
bool Stage::load(uint32_t number)
{
    char* path = new char[1024]();
    
    snprintf(path, 1024, "%s/stages/Stage%d.json", SWDataPath, number);
    
    pinfo("Stage file is %s.", path);
    
    // Read the stage data
    std::ifstream sfile(path);
    
    delete [] path;
    
    if (!sfile.good())
    {
        pserror("Failed to load the stage data for #%d.", number);
        
        return false;
    }
    
    JSON object;
    
    sfile >> object;
    
    // Decode the stage data
    char keybuf[64];
    
    // A convenient lambda function that takes the coding key and submarine type, and returns a float value
    auto getFloat = [keybuf, object] (Stage::Keys::Key key, Submarine::Type type) mutable -> float
    {
        Keys::get(keybuf, size(keybuf), key, type);
        
        return object[keybuf].get<float>();
    };
    
    for (auto& type : Submarine::allTypes)
    {
        uint32_t index = Submarine::typeToIndex(type);
        
        //pinfo("Type%d: Vel L = %f; U = %f.", index, getFloat(Keys::SubVelRangeLower, type), getFloat(Keys::SubVelRangeUpper, type));
        
        this->subvranges.emplace(type, Range<float>(getFloat(Keys::SubVelRangeLower, type),
                                                    getFloat(Keys::SubVelRangeUpper, type)));
        
        //pinfo("Type%d: Ycd L = %f; U = %f.", index, getFloat(Keys::SubYcoordLower, type), getFloat(Keys::SubYcoordUpper, type));
        
        this->subyranges.emplace(type, Range<float>(getFloat(Keys::SubYcoordLower, type),
                                                    getFloat(Keys::SubYcoordUpper, type)));
        
        //pinfo("Type%d: RDR L = %f; U = %f.", index, getFloat(Keys::SubRadarRadiusRangeLower, type), getFloat(Keys::SubRadarRadiusRangeUpper, type));
        
        this->subrrranges.emplace(type, Range<float>(getFloat(Keys::SubRadarRadiusRangeLower, type),
                                                     getFloat(Keys::SubRadarRadiusRangeUpper, type)));
        
        Keys::get(keybuf, size(keybuf), Keys::SubCountLimit, type);
        
        //pinfo("Type%d: CLT = %d.", index, object[keybuf].get<int>());
        
        this->subcounts.emplace(type, object[keybuf].get<int>());

        //Keys::get(keybuf, size(keybuf), Keys::FishCount, type);

        //this->fish.emplace(type, object[keybuf].get<int>());
    }


    this->fishcount = getFloat(Keys::FishCount, Submarine::Type::I);
    float str = getFloat(Keys::StageType, Submarine::Type::I);
    this->sType = (int) str;
    this->currentVec.x = getFloat(Keys::CurrVec, Submarine::Type::I);
    printf("Read current %f\n", getFloat(Keys::CurrVec, Submarine::Type::I));
    this->currentVec.y = getFloat(Keys::CurrVec, Submarine::Type::II);

    // TODO: IMP THIS
    // Decoding required control data later, e.g. Attack AI data, etc.
    return true;
}
