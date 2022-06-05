# RfgPatch
This tool installs map mods for Red Faction Guerrilla Re-Mars-tered. The patches are generated by the map editor in [Nanoforge](https://github.com/Moneyl/Nanoforge/), which currently is pre release testing. RfgPatch is a temporary tool. We don't want to distribute entire map packfiles since that includes many unedited assets like textures. RFG has a mod manager, but it's currently not fit for installing maps, and is closed source. RfgPatch will be replaced once the mod manager is rewritten in the next year or so.

# How to use
- Download RfgPatch from the [releases page](https://github.com/Moneyl/RfgPatch/releases/)
- Copy RfgPatch.exe to your RFGR folder. This is usually in `C:/Program Files (x86)/Steam/steamapps/common/Red Faction Guerrilla Re-Mars-Tered/`. You can also check in the steam library by right clicking the game and clicking `Properties > Local Files > Browse...`
- Drag and drop a .RfgPatch onto RfgPatch.exe. It'll install the patch and backup the original version of the vpp_pc it changes. For example, if it patches `mp_crescent.vpp_pc` it'll backup the original as `mp_crescent.original.vpp_pc`. You can also select multiple patch files and drop them onto RfgPatch.exe to install them all