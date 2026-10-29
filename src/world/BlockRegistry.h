#pragma once

#include <string>

using BlockID = uint16_t;

struct BlockDef {
    std::string name;
    bool solid = true;
    bool opaque = true;
    int tileTop = 0;
    int tileBottom = 0;
    int tileSide = 0;
};

class BlockRegistry {
    public:
        BlockRegistry() {
            buildDefaultRegistry();
        }

        void add(BlockDef def) {
            if (frozen) {
                throw 1;
            }

            byName.insert({def.name, BlockID(defs.size())});
            defs.push_back(def);
        }

        void freeze() {
            frozen = true;
        }

        const BlockDef& get(BlockID id) const {
            return defs.at(id);
        }

        BlockID idOf(const std::string& name) const {
            for (auto& [blockName, blockID] : byName) {
                if (blockName == name) {
                    return blockID;
                }
            }
            return 0;
        }

        int tileFor(BlockID id, int face) const {
            BlockDef bd = get(id);
            switch (face){
                case 2:
                    return bd.tileTop;
                    break;

                case 3:
                    return bd.tileBottom;
                    break;

                default:
                    return bd.tileSide;
                    break;
            }

        }

    private:
        std::vector<BlockDef> defs;
        std::unordered_map<std::string, BlockID> byName;
        bool frozen = false;

        void buildDefaultRegistry() {
            add(BlockDef{"air", false, false, 0, 0, 0});
            add(BlockDef{"stone", true, true, 7, 7, 7});
            add(BlockDef{"dirt", true, true, 21, 21, 21});
            add(BlockDef{"grass", true, true, 32, 21, 31});
        }

};