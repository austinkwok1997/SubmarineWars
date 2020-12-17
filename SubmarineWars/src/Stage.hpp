//
//  Stage.hpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-10-12.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef Stage_hpp
#define Stage_hpp

#include "Foundations/Foundations.hpp"
#include "Entities/Submarine.hpp"
#include <unordered_map>
#include <string.h>

/// Represents a stage in the game
class Stage
{
public:
    ///
    /// Load a stage
    ///
    /// @param number The stage number
    ///
    bool load(uint32_t number);
    
    //
    // MARK:- Query Stage Control Data
    //
    
    ///
    /// [FAST] Check whether this stage is loaded or not
    ///
    inline bool isLoaded()
    {
        return this->loaded;
    }
    
    ///
    /// [FAST] Get the stage number
    ///
    inline uint32_t getStageNumber()
    {
        return this->number;
    }
    
    ///
    /// [FAST] Get the velocity range for the given submarine type
    ///
    inline const Range<float>& getSubmarineVelocityRange(Submarine::Type type)
    {
        return this->subvranges[type];
    }
    
    ///
    /// [FAST] Get the y-coordinate range for the given submarine type
    ///
    inline const Range<float>& getSubmarineYcoordRange(Submarine::Type type)
    {
        return this->subyranges[type];
    }
    
    ///
    /// [FAST] Get the radar radius range for the given submarine type
    ///
    inline const Range<float>& getSubmarineRadarRadiusRange(Submarine::Type type)
    {
        return this->subrrranges[type];
    }
    
    ///
    /// [FAST] Get the upper limit of the total number of submarines
    ///
    /// @note A limit value of 0 indicates no limit.
    ///       This is useful for building a tutorial stage or the endless mode
    ///
    inline const std::unordered_map<Submarine::Type, uint32_t>& getSubmarineCountLimits()
    {
        return this->subcounts;
    }

    ///
    /// Get the number of fish to spawn for this stage
    ///
    inline const uint32_t& getFishCount() {
        return this->fishcount;
    }

    ///
    /// Get the water current for this stage
    ///
    inline const vec2& getCurrent() {
        printf("Current vec is %f\n", this->currentVec.x);
        return this->currentVec;
    }

    ///
    /// Get the type of the current stage
    /// 0 = normal
    /// 1 = store
    inline const int& getStageType() {
        return this->sType;
    }

    
private:
    /// The stage number
    uint32_t number;
    
    /// Indicate whether this stage is loaded or not
    bool loaded;
    
    /// The velocity range of all types of submarines
    std::unordered_map<Submarine::Type, Range<float>> subvranges;
    
    /// The y-coordinate range of all types of submarines
    std::unordered_map<Submarine::Type, Range<float>> subyranges;
    
    /// The radar radis range of all types of submarines
    std::unordered_map<Submarine::Type, Range<float>> subrrranges;
    
    /// The total number of all types of submarines in this stage
    std::unordered_map<Submarine::Type, uint32_t> subcounts;

    /// The total number of fish in this stage
    uint32_t fishcount;

    /// The current vector of this stage
    vec2 currentVec;

    /// Gets the type of this stage
    int sType;
    
    /// Enumerates all keys used in encoding and decoding stage control data
    struct Keys
    {
        enum Key
        {
            /// Submarine Velocity Range: Lower bound
            SubVelRangeLower,
            
            /// Submarine Velocity Range: Upper bound
            SubVelRangeUpper,
            
            /// Submarine Y-coordinate Range: Lower bound
            SubYcoordLower,
            
            /// Submarine Y-coordinate Range: Upper bound
            SubYcoordUpper,
            
            /// Submarine Radar Radius Range: Lower bound
            SubRadarRadiusRangeLower,
            
            /// Submarine Radar Radius Range: Upper bound
            SubRadarRadiusRangeUpper,
            
            /// Submarine Count Limit
            SubCountLimit,

            /// Fish count
            FishCount,

            /// Current vector. Use type 1 for x component, type 2 for y component
            CurrVec,

            /// The type of the current stage
            /// 0 = Normal
            /// 1 = Store
            StageType
        };
        
        ///
        /// [Helper] Get the format string for the given decoding key
        ///
        static inline const char* formatForKey(Stage::Keys::Key key)
        {
            switch (key)
            {
                case SubVelRangeLower:
                    return "Sub%dVelRl";
                    
                case SubVelRangeUpper:
                    return "Sub%dVelRu";
                    
                case SubYcoordLower:
                    return "Sub%dYcoordRl";
                    
                case SubYcoordUpper:
                    return "Sub%dYcoordRu";
                    
                case SubRadarRadiusRangeLower:
                    return "Sub%dRadarRadiusRl";
                    
                case SubRadarRadiusRangeUpper:
                    return "Sub%dRadarRadiusRu";
                    
                case SubCountLimit:
                    return "Sub%dCountLimit";

                case FishCount:
                    return "FishCountLimit";

                case CurrVec:
                    return "CurrentVector%d";

                case StageType:
                    return "StageType";

                default:
                    pserror("[Fatal] Unimplemented switch case.");
                    
                    return nullptr;
            }
        }
        
        ///
        /// Get the coding key for the given submarine type
        ///
        /// @param buffer A non-null buffer that contains the key on return
        /// @param size The size of the given buffer
        /// @param key The submarine type dependent key
        /// @param subType The submarine type
        /// @note This function will clear the given buffer.
        ///
        static inline void get(char* buffer, size_t size, Stage::Keys::Key key, Submarine::Type subType)
        {
            memset(buffer, 0, size);
            
            snprintf(buffer, size, formatForKey(key), Submarine::typeToIndex(subType));
        }
    };
};

#endif /* Stage_hpp */
