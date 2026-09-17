#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <array>

namespace
{
    using sds::WeaponSpeakerArchivePolicy;
    using sds::WeaponSpeakerCue;
    using sds::WeaponSpeakerProfile;
    using sds::WeaponSpeakerSustainedCue;
    using sds::WeaponSpeakerTrigger;

    constexpr float kBatchGain = 0.35F;
    constexpr float kStarstormSustainedBodyGain = 1.60F;
    constexpr std::size_t kFireMaxFrames = 28800u;
    constexpr std::size_t kFireFadeFrames = 480u;

    const std::array<WeaponSpeakerProfile, 49> kProfiles{
        WeaponSpeakerProfile{
            "Maelstrom",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xE7814E8Eu, 0u, 0u, false, 0.35F, 28800u, 480u,
                    {
                        { 1u, "WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_01.wav", WeaponSpeakerArchivePolicy::RequirePatch },
                        { 2u, "WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_02.wav", WeaponSpeakerArchivePolicy::RequirePatch },
                        { 3u, "WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_03.wav", WeaponSpeakerArchivePolicy::RequirePatch },
                        { 4u, "WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_04.wav", WeaponSpeakerArchivePolicy::RequirePatch },
                        { 5u, "WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_05.wav", WeaponSpeakerArchivePolicy::RequirePatch },
                        { 6u, "WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_06.wav", WeaponSpeakerArchivePolicy::RequirePatch },
                    } },
                WeaponSpeakerCue{
                    "bolt-out", WeaponSpeakerTrigger::WwisePost,
                    0x7F65DE86u, 0x7F65DE86u, 0x2u, true, 0.35F, 0u, 0u,
                    { { 1u, "WPM_Maelstrom_Reload_Bolt_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{
                    "clip-out", WeaponSpeakerTrigger::WwisePost,
                    0xEBD95A39u, 0xEBD95A39u, 0x2u, true, 0.35F, 0u, 0u,
                    {
                        { 1u, "WPM_Maelstrom_Reload_Clip_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPM_Maelstrom_Reload_Clip_Out_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{
                    "clip-in", WeaponSpeakerTrigger::WwisePost,
                    0x7A821716u, 0x7A821716u, 0x2u, true, 0.35F, 0u, 0u,
                    {
                        { 1u, "WPM_Maelstrom_Reload_Clip_In_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPM_Maelstrom_Reload_Clip_In_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{
                    "draw", WeaponSpeakerTrigger::WwisePost,
                    0xFFDDC978u, 0xFFDDC978u, 0x2u, true, 0.35F, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Rifle_Maelstrom_Equip_Up_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Maelstrom_Equip_Up_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Maelstrom_Equip_Up_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{
                    "holster", WeaponSpeakerTrigger::WwisePost,
                    0x5A51678Fu, 0x5A51678Fu, 0x2u, true, 0.35F, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Rifle_Maelstrom_Equip_Down_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Maelstrom_Equip_Down_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Maelstrom_Equip_Down_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
            } },
        WeaponSpeakerProfile{
            "Grendel",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x242CBC48u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Grendel_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Grendel_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Grendel_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xD7B01A91u, 0xD7B01A91u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Grendel_Reload_1MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x5CC56ED3u, 0x5CC56ED3u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Grendel_Reload_2MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0xB1423DBFu, 0xB1423DBFu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Grendel_Reload_3BoltClose_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x1C2A8A25u, 0x1C2A8A25u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Grendel_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x24C05CFEu, 0x24C05CFEu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Grendel_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Beowulf",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x388FCA0Eu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Beowulf_Fire_PC_V3_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Beowulf_Fire_PC_V3_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Beowulf_Fire_PC_V3_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_Beowulf_Fire_PC_V3_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_Beowulf_Fire_PC_V3_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 6u, "WPN_Hand_Rifle_Beowulf_Fire_PC_V3_06.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x0FB87E7Au, 0x0FB87E7Au, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Beowulf_Reload_MagOut_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-a", WeaponSpeakerTrigger::WwisePost,
                    0xD42898CAu, 0xD42898CAu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Beowulf_Reload_MagIn_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-b", WeaponSpeakerTrigger::WwisePost,
                    0x5C94E669u, 0x5C94E669u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Beowulf_Reload_MagIn_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xEE0EA966u, 0xEE0EA966u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Beowulf_Equip_FirstPerson_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x074A4E21u, 0x074A4E21u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Beowulf_Equip_FirstPerson_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Kodama",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xC65C464Eu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Kodama_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Kodama_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Kodama_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "reload-start", WeaponSpeakerTrigger::WwisePost,
                    0x3832846Fu, 0x3832846Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Kodama_Reload_01_Start_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0x5A23E79Cu, 0x5A23E79Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Kodama_Reload_02_Bolt_Open_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xF55210EDu, 0xF55210EDu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Kodama_Reload_03_Mag_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x9599E02Fu, 0x9599E02Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Kodama_Reload_04_Mag_In_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x95746B98u, 0x95746B98u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Kodama_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xC517F4AFu, 0xC517F4AFu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Kodama_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Urban Eagle",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x7B188A09u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_UrbanEagle_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_UrbanEagle_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_UrbanEagle_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Pistol_UrbanEagle_Fire_PC_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Pistol_UrbanEagle_Fire_PC_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x04A8FBF0u, 0x04A8FBF0u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_UrbanEagle_Reload_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x4F8F5BD1u, 0x4F8F5BD1u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_UrbanEagle_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-slap", WeaponSpeakerTrigger::WwisePost,
                    0x3FA33946u, 0x3FA33946u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Reload_MagSlap_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x725A67EEu, 0x725A67EEu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_UrbanEagle_Reload_BoltClose_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x88520D3Eu, 0x88520D3Eu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x4484F819u, 0x4484F819u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Coachman",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xA478F7E4u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Shotgun_Coachman_Fire_PC_Semi_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Shotgun_Coachman_Fire_PC_Semi_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Shotgun_Coachman_Fire_PC_Semi_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0x9AC534D0u, 0x9AC534D0u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Coachman_Reload_1_BoltOpen_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x259615F3u, 0x259615F3u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Coachman_Reload_2_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xC9023A7Du, 0xC9023A7Du, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Coachman_Reload_3_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0xFE8C2DE9u, 0xFE8C2DE9u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Coachman_Reload_4_BoltClose_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x343BA5C1u, 0x343BA5C1u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Coachman_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xFCE39DA2u, 0xFCE39DA2u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Coachman_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Breach",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x8AD0FBCFu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Shotgun_Breach_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Shotgun_Breach_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Shotgun_Breach_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out-a", WeaponSpeakerTrigger::WwisePost,
                    0x546FD842u, 0x546FD842u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Breach_1MagOut_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out-b", WeaponSpeakerTrigger::WwisePost,
                    0xA42BB7E8u, 0xA42BB7E8u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Breach_2MagOut_B_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xC7518661u, 0xC7518661u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Breach_3MagIn_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x0D8FAE45u, 0x0D8FAE45u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Breach_EquipUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xA039691Eu, 0xA039691Eu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Breach_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Magshot",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xC7DA88B5u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_MagShot_Player_Fire_Semi_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_MagShot_Player_Fire_Semi_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_MagShot_Player_Fire_Semi_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0xBC587E87u, 0xBC587E87u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_MagShot_Reload_01_Bolt_Open_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xE487A100u, 0xE487A100u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_MagShot_Reload_02_Mag_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xB1789B8Eu, 0xB1789B8Eu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_MagShot_Reload_03_Mag_In_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x7F6B98D6u, 0x7F6B98D6u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_MagShot_Tumbler_Rotate_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x004C7EFFu, 0x004C7EFFu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_MagShot_Tumbler_Rotate_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Equinox",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x9A887E27u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 6u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_06.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out-1", WeaponSpeakerTrigger::WwisePost,
                    0x4E120147u, 0x4E120147u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out-2", WeaponSpeakerTrigger::WwisePost,
                    0xF07C4522u, 0xF07C4522u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-3", WeaponSpeakerTrigger::WwisePost,
                    0xE4D2DA06u, 0xE4D2DA06u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-4", WeaponSpeakerTrigger::WwisePost,
                    0xA5D78B1Du, 0xA5D78B1Du, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-5", WeaponSpeakerTrigger::WwisePost,
                    0x962AA8F4u, 0x962AA8F4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x94A39E82u, 0x94A39E82u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Equip_Up_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xF16D6275u, 0xF16D6275u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Equip_Down_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Va'ruun Starlash",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x9A887E27u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 6u, "WPN_Hand_Rifle_Equinox_PC_Fire_Semi_06.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out-1", WeaponSpeakerTrigger::WwisePost,
                    0x4E120147u, 0x4E120147u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out-2", WeaponSpeakerTrigger::WwisePost,
                    0xF07C4522u, 0xF07C4522u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-3", WeaponSpeakerTrigger::WwisePost,
                    0xE4D2DA06u, 0xE4D2DA06u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-4", WeaponSpeakerTrigger::WwisePost,
                    0xA5D78B1Du, 0xA5D78B1Du, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-5", WeaponSpeakerTrigger::WwisePost,
                    0x962AA8F4u, 0x962AA8F4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x94A39E82u, 0x94A39E82u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Equip_Up_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xF16D6275u, 0xF16D6275u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Equinox_Equip_Down_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            }, "Equinox" },
        WeaponSpeakerProfile{
            "Big Bang",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xDAB9706Eu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Shotgun_Bigbang_PC_Fire_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Shotgun_Bigbang_PC_Fire_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "insert-mag", WeaponSpeakerTrigger::WwisePost,
                    0x9C213D33u, 0x9C213D33u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_BigBang_Reload_InsertMag_3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "slam-mag", WeaponSpeakerTrigger::WwisePost,
                    0x97F40BE0u, 0x97F40BE0u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_BigBang_Reload_SlamMag_4.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x148328B5u, 0x148328B5u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_BigBang_Equip.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x24E28A22u, 0x24E28A22u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_BigBang_Unequip.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Shotty",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x867E0D9Au, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Shotgun_Shotty_Fire_PC_Semi_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Shotgun_Shotty_Fire_PC_Semi_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Shotgun_Shotty_Fire_PC_Semi_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xBCAC5FCDu, 0xBCAC5FCDu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Shotty_2MagIn_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0x7DA20107u, 0x7DA20107u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Shotty_3BoltOpen_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x9D39079Cu, 0x9D39079Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Shotty_4BoltClose_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x1A40875Cu, 0x1A40875Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Shotty_EquipUp_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x4479F8BBu, 0x4479F8BBu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Shotty_EquipDown_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Eon",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x58F7EF25u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Pistol_Eon_Fire_Ball_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Eon_Fire_Ball_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Pistol_Eon_Fire_Ball_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Pistol_Eon_Fire_Ball_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xAA2B3F43u, 0xAA2B3F43u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Reload_MagOut_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x847E55A4u, 0x847E55A4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Reload_MagIn_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-slap", WeaponSpeakerTrigger::WwisePost,
                    0xFD0067F9u, 0xFD0067F9u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Reload_MagSlap_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x7D5840F7u, 0x7D5840F7u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x3471125Cu, 0x3471125Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Sidestar",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xBDDE8E32u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_PIstol_Sidestar_PC_Semi_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_PIstol_Sidestar_PC_Semi_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_PIstol_Sidestar_PC_Semi_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_PIstol_Sidestar_PC_Semi_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_PIstol_Sidestar_PC_Semi_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x1CFC2D1Au, 0x1CFC2D1Au, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_PIstol_Sidestar_1stPerson_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xC264A94Du, 0xC264A94Du, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_PIstol_Sidestar_1stPerson_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xFF8FFF7Au, 0xFF8FFF7Au, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-check", WeaponSpeakerTrigger::WwisePost,
                    0xC43F4032u, 0xC43F4032u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Sidestar_Reload_03_Bolt_Close_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xAB115B3Fu, 0xAB115B3Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Rattler",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x86D8BAF3u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Pistol_Rattler_Fire_Ball_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Rattler_Fire_Ball_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Pistol_Rattler_Fire_Ball_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Pistol_Rattler_Fire_Ball_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xD667E768u, 0xD667E768u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Rattler_Reload_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x0D5C60C9u, 0x0D5C60C9u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Rattler_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x12D086B6u, 0x12D086B6u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Rattler_BoltClose_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x86969E70u, 0x86969E70u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Rattler_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xE92B9BA7u, 0xE92B9BA7u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Rattler_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Old Earth Pistol",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x81E1C425u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_1919_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_1919_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_1919_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x50F61B13u, 0x50F61B13u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_1919_Reload_1MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x51E83AEDu, 0x51E83AEDu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_1919_Reload_2MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x2DD4A70Cu, 0x2DD4A70Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_1911_EquipUp_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x02F705C4u, 0x02F705C4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_1919_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "XM-2311",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x81E1C425u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_1919_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_1919_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_1919_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x50F61B13u, 0x50F61B13u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_1919_Reload_1MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x51E83AEDu, 0x51E83AEDu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_1919_Reload_2MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x2DD4A70Cu, 0x2DD4A70Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_1911_EquipUp_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x02F705C4u, 0x02F705C4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_1919_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            },
            "Old Earth Pistol" },
        WeaponSpeakerProfile{
            "Kraken",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xF7DB6C42u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_Kraken_Player_Fire_Blast_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Kraken_Player_Fire_Blast_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_Kraken_Player_Fire_Blast_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "reload-start", WeaponSpeakerTrigger::WwisePost,
                    0x06EAD01Au, 0x06EAD01Au, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Kraken_Reload_Start_1.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mechanism-open", WeaponSpeakerTrigger::WwisePost,
                    0x18AEF50Bu, 0x18AEF50Bu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Kraken_Reload_Mechanism_Open_2.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xB44ACB3Bu, 0xB44ACB3Bu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Kraken_Reload_Insert_Mag_3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "hand-bump", WeaponSpeakerTrigger::WwisePost,
                    0xCCB08E49u, 0xCCB08E49u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Kraken_Reload_Hand_Bump_4.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x1A3783BDu, 0x1A3783BDu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Kraken_Equip_Start_1.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xB767B53Fu, 0xB767B53Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Kraken_Unequip.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Regulator",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xAAD5827Cu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_Regulator_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Regulator_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_Regulator_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x45C59F44u, 0x45C59F44u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Regulator_Reload_1MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xEE4E4FA4u, 0xEE4E4FA4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Regulator_Reload_2MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x4DF40920u, 0x4DF40920u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Regulator_EquipUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x6E08BE77u, 0x6E08BE77u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_1919_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Razorback",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x0C6B0ED2u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_Razorback_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Razorback_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_Razorback_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0x9843EA3Du, 0x9843EA3Du, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Razorback_Reload_1BoltOpen_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x752E8EADu, 0x752E8EADu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Razorback_Reload_2MagIn_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x406EC72Du, 0x406EC72Du, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Razorback_Reload_3BoltClose_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xB0CC01C3u, 0xB0CC01C3u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Pistol_Razorback_EquipUp_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xA83CA7C0u, 0xA83CA7C0u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "wpn_hand_pistol_razorback_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "AA-99",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xB671D357u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_AA99_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_AA99_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_AA99_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0x7543B1C6u, 0x7543B1C6u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_RifleAA99_Reload_1BoltOpen_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xDEAFFD73u, 0xDEAFFD73u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_RifleAA99_Reload_2MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x74A49914u, 0x74A49914u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_RifleAA99_Reload_3MagIn_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x718689EBu, 0x718689EBu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_RifleAA99_Reload_5BoltClose_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x3A92DB10u, 0x3A92DB10u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_AA99_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x6C3F8747u, 0x6C3F8747u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_AA99_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Drum Beat",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x9C2277ADu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Combatech_Drumbeat_Fire_PC_Semi_V3_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Combatech_Drumbeat_Fire_PC_Semi_V3_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Combatech_Drumbeat_Fire_PC_Semi_V3_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_Combatech_Drumbeat_Fire_PC_Semi_V3_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_Combatech_Drumbeat_Fire_PC_Semi_V3_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 6u, "WPN_Hand_Rifle_Combatech_Drumbeat_Fire_PC_Semi_V3_06.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 7u, "WPN_Hand_Rifle_Combatech_Drumbeat_Fire_PC_Semi_V3_07.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 8u, "WPN_Hand_Rifle_Combatech_Drumbeat_Fire_PC_Semi_V3_08.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xC7D6A881u, 0xC7D6A881u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Combatech_Drumbeat_Reload_MagIn_PC_01_V2.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x2BD8A31Fu, 0x2BD8A31Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Combatech_Drumbeat_Equip_Up_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xCC825DE4u, 0xCC825DE4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Combatech_Drumbeat_Equip_Down_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Tombstone",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x82412C8Eu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Tombstone_Fire_PC_Semi_V2_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Tombstone_Fire_PC_Semi_V2_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Tombstone_Fire_PC_Semi_V2_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_Tombstone_Fire_PC_Semi_V2_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_Tombstone_Fire_PC_Semi_V2_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 6u, "WPN_Hand_Rifle_Tombstone_Fire_PC_Semi_V2_06.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 7u, "WPN_Hand_Rifle_Tombstone_Fire_PC_Semi_V2_07.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 8u, "WPN_Hand_Rifle_Tombstone_Fire_PC_Semi_V2_08.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xBA06E4F9u, 0xBA06E4F9u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Tombstone_MagOut_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x2F25B050u, 0x2F25B050u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Tombstone_MagIn_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x9B7604ADu, 0x9B7604ADu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Tombstone_EquipUp_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xF2FE2B86u, 0xF2FE2B86u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Tombstone_EquipDown_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Old Earth Assault Rifle",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x6AC713CFu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_RussianAssault_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_RussianAssault_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_RussianAssault_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xD472F9A4u, 0xD472F9A4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_RussianAssault_1MagOut_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xB6D6AD84u, 0xB6D6AD84u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_RussianAssault_2MagIn_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0x225BCCE4u, 0x225BCCE4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_RussianAssault_3BoltOpen_01_L.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xF733A291u, 0xF733A291u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Tombstone_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x820A5392u, 0x820A5392u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Tombstone_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Lawgiver",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x17505423u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Lawgiver_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Lawgiver_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Lawgiver_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xBA9DF2B5u, 0xBA9DF2B5u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Lawgiver_Reload_02_MagOut_f21_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x95571646u, 0x95571646u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Lawgiver_Reload_03_MagIn_f59_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xF2194792u, 0xF2194792u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Lawgiver_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x1B583D72u, 0x1B583D72u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Lawgiver_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Old Earth Hunting Rifle",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x7C052BF5u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_HuntingRussian_Fire_PC_Mod_Suppressor_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_HuntingRussian_Fire_PC_Mod_Suppressor_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_HuntingRussian_Fire_PC_Mod_Suppressor_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x3508CD70u, 0x3508CD70u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_HuntingRussian_2MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "adjust", WeaponSpeakerTrigger::WwisePost,
                    0xB7EE376Cu, 0xB7EE376Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_HuntingRussian_Adjust_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0xB7107FE0u, 0xB7107FE0u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_HuntingRussian_3BoltOpen.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0xA2A22CB5u, 0xA2A22CB5u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_HuntingRussian_4BoltClose_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x79A15C54u, 0x79A15C54u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Hunting_Russian_EquipUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x42C6F6D3u, 0x42C6F6D3u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Hunting_Russian_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Hard Target",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x4210BAB4u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_HardTarget_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_HardTarget_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_HardTarget_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x21F29CFFu, 0x21F29CFFu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_HardTarget_3MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x0239F801u, 0x0239F801u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_HardTarget_4BoltClose_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xD27DBF68u, 0xD27DBF68u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_HardTarget_EquipUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x82C5C5DFu, 0x82C5C5DFu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_HardTarget_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Old Earth Shotgun",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x47C05B2Eu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Shotgun_Pump_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Shotgun_Pump_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Shotgun_Pump_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "bolt-charge-a", WeaponSpeakerTrigger::WwisePost,
                    0x5A5B7BE9u, 0x5A5B7BE9u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pump_Reload_1BoltCharge_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-charge-b", WeaponSpeakerTrigger::WwisePost,
                    0x5A5B7BEAu, 0x5A5B7BEAu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pump_Reload_1BoltCharge_B_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-charge-c", WeaponSpeakerTrigger::WwisePost,
                    0x5A5B7BEBu, 0x5A5B7BEBu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pump_Reload_1BoltCharge_C_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x3FD744D2u, 0x3FD744D2u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pump_Reload_2MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x97C57006u, 0x97C57006u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pump_EquipUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x4DA4D3C1u, 0x4DA4D3C1u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pump_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Pacifier",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xEFDCBBFFu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Shotgun_Pacifier_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Shotgun_Pacifier_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Shotgun_Pacifier_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out-a", WeaponSpeakerTrigger::WwisePost,
                    0xB2E1E9D2u, 0xB2E1E9D2u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pacifier_1MagOut_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out-b", WeaponSpeakerTrigger::WwisePost,
                    0xA15FF5F8u, 0xA15FF5F8u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pacifier_2MagOut_B_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-a", WeaponSpeakerTrigger::WwisePost,
                    0x6C365251u, 0x6C365251u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pacifier_3MagIn_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-b", WeaponSpeakerTrigger::WwisePost,
                    0x553FC75Du, 0x553FC75Du, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pacifier_4MagIn_B_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x3142CDD5u, 0x3142CDD5u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pump_EquipUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x7E011A8Eu, 0x7E011A8Eu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Shotgun_Pump_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Auto-Rivet",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xCDEE7F71u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_AutoRivet_PC_Motor_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_AutoRivet_PC_Motor_Out_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_AutoRivet_PC_Motor_Out_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_AutoRivet_PC_Motor_Out_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xB0285D9Fu, 0xB0285D9Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagStorm_Reload_04_Mag_In_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x8291BED0u, 0x8291BED0u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagStorm_Reload_05_Bolt_Close_01_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xB095127Fu, 0xB095127Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_AutoRivet_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xE7269144u, 0xE7269144u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_AutoRivet_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Microgun",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xC0D50831u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_AlliedMicrogun_Fire_Player_Auto_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_AlliedMicrogun_Fire_Player_Auto_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_AlliedMicrogun_Fire_Player_Auto_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_AlliedMicrogun_Fire_Player_Auto_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_AlliedMicrogun_Fire_Player_Auto_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "spin-up", WeaponSpeakerTrigger::WwisePost,
                    0x05414BB7u, 0x05414BB7u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_AlliedMicrogun_SpinUp.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "spin-down", WeaponSpeakerTrigger::WwisePost,
                    0x1A1F9A1Cu, 0x1A1F9A1Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_AlliedMicrogun_SpinDown.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xF6F5FF2Eu, 0xF6F5FF2Eu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Microgun_Reload_2_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x66F5223Au, 0x66F5223Au, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Microgun_Reload_3_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-rack", WeaponSpeakerTrigger::WwisePost,
                    0xCEFDE727u, 0xCEFDE727u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Microgun_Reload_4_BoltRack_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xABED51B2u, 0xABED51B2u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Microgun_EquipUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x66A77445u, 0x66A77445u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Microgun_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Bridger",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x1D03DEB5u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Launcher_Bridger_Fire_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Launcher_Bridger_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Launcher_Bridger_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "bolt-charge-a", WeaponSpeakerTrigger::WwisePost,
                    0x6FC3006Au, 0x6FC3006Au, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Launcher_Bridger_Reload_1BoltCharge_A_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-charge-b", WeaponSpeakerTrigger::WwisePost,
                    0x6FC30069u, 0x6FC30069u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Launcher_Bridger_Reload_1BoltCharge_B_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x2342C39Du, 0x2342C39Du, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Launcher_Bridger_Reload_2MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xD9D67E2Fu, 0xD9D67E2Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_LauncherBridger_EquipUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x7570F154u, 0x7570F154u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_LauncherBridger_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Negotiator",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xD92F5705u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rocketlauncher_Fire_PC_Semi_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rocketlauncher_Fire_PC_Semi_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rocketlauncher_Fire_PC_Semi_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x72082460u, 0x72082460u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_RocketLauncher_Reload_2_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x7E5DE8F8u, 0x7E5DE8F8u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_RocketLauncher_Reload_3_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-slap", WeaponSpeakerTrigger::WwisePost,
                    0x8C9E4A0Cu, 0x8C9E4A0Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_RocketLauncher_Reload_3a_MagSlap_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x6D7352F8u, 0x6D7352F8u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_RocketLauncher_EquipUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xA32DCD0Fu, 0xA32DCD0Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_RocketLauncher_EquipDown_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Magshear",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x41B2B9D5u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_MagShear_Player_Fire_Auto_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_MagShear_Player_Fire_Auto_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_MagShear_Player_Fire_Auto_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_MagShear_Player_Fire_Auto_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_MagShear_Player_Fire_Auto_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0x19044091u, 0x19044091u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagShear_Reload_02_Bolt_Open_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x3AC8B094u, 0x3AC8B094u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagShear_Reload_03_Mag_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xC2017DB4u, 0xC2017DB4u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagShear_Reload_04_Mag_In_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x961B4C96u, 0x961B4C96u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagShear_Reload_05_Bolt_Close_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "reload-end", WeaponSpeakerTrigger::WwisePost,
                    0x1C0294CEu, 0x1C0294CEu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagShear_Reload_06_End_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x8CD475BBu, 0x8CD475BBu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagShear_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xC671D918u, 0xC671D918u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagShear_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Magpulse",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x2FB62C31u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_MagPulse_Player_Fire_Semi_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x15A90DA2u, 0x15A90DA2u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagPulse_Reload_02_Mag_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x671FBA12u, 0x671FBA12u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagPulse_Reload_03_Mag_In_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x2D46E1BAu, 0x2D46E1BAu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagPulse_Reload_04_Bolt_Close_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x0CA8434Fu, 0x0CA8434Fu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagPulse_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xCFAC7074u, 0xCFAC7074u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagPulse_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Magsniper",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x2D47CA9Cu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_MagSniper_Player_Fire_Semi_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_MagSniper_Player_Fire_Semi_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_MagSniper_Player_Fire_Semi_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x92F413C1u, 0x92F413C1u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagSniper_Reload_02_Mag_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-a", WeaponSpeakerTrigger::WwisePost,
                    0x899AD555u, 0x899AD555u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagSniper_Reload_03_Mag_In_01_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in-b", WeaponSpeakerTrigger::WwisePost,
                    0x9E97A530u, 0x9E97A530u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagSniper_Reload_03_Mag_In_02_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x6C0ABE53u, 0x6C0ABE53u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagSniper_Equip_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x36308030u, 0x36308030u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagSniper_UnEquip_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Magstorm",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xCC6AFF22u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_MagStorm_Player_Fire_Auto_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_MagStorm_Player_Fire_Auto_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_MagStorm_Player_Fire_Auto_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_MagStorm_Player_Fire_Auto_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_MagStorm_Player_Fire_Auto_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0x5BCC74BBu, 0x5BCC74BBu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagStorm_Reload_02_Bolt_Open_02_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xFAE6B259u, 0xFAE6B259u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagStorm_Reload_03_Mag_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x2A88D4F9u, 0x2A88D4F9u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagStorm_Reload_04_Mag_In_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0xE7917B71u, 0xE7917B71u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_MagStorm_Reload_05_Bolt_Close_01_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x2670A99Cu, 0x2670A99Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagStorm_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x7B8424FBu, 0x7B8424FBu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_MagStorm_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Solstice",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xDA2C534Au, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 6u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_06.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xCD519327u, 0xCD519327u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Solstice_Reload_MagOut_V2_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Solstice_Reload_MagOut_V2_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-grab", WeaponSpeakerTrigger::WwisePost,
                    0x9240A702u, 0x9240A702u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Solstice_Reload_MagOut_pt2_V2_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Solstice_Reload_MagOut_pt2_V2_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xF9D4D3DAu, 0xF9D4D3DAu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Solstice_Reload_MagIn_V2_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Solstice_Reload_MagIn_V2_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x28FC1D9Eu, 0x28FC1D9Eu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Combatech_Solstice_Equip_Up_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Combatech_Solstice_Equip_Up_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x56D49E79u, 0x56D49E79u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Combatech_Solstice_Equip_Down_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Combatech_Solstice_Equip_Down_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
            } },
        WeaponSpeakerProfile{
            "Orion",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x926F52DAu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Orion_Fire_v8_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Orion_Fire_v8_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Orion_Fire_v8_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_Orion_Fire_v8_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_Orion_Fire_v8_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xA52BB1D3u, 0xA52BB1D3u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Reload_1_MagOut_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x262CFE9Bu, 0x262CFE9Bu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Reload_2_MagIn_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "raise", WeaponSpeakerTrigger::WwisePost,
                    0xFD260E36u, 0xFD260E36u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Reload_3_RaiseGun_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xF68897F7u, 0xF68897F7u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Equip_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xA0BAC15Cu, 0xA0BAC15Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Unequip_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Novalight",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x2E3CCCFDu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_Novalight_Fire_PC_01  .wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Novalight_Fire_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_Novalight_Fire_PC_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x4CD322DEu, 0x4CD322DEu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Pistol_Novalight_Reload_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Novalight_Reload_MagOut_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xF0A32E63u, 0xF0A32E63u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Pistol_Novalight_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Novalight_Reload_MagIn_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "hand-bump", WeaponSpeakerTrigger::WwisePost,
                    0x1DA93682u, 0x1DA93682u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Pistol_Novalight_Reload_HandBump_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Novalight_Reload_HandBump_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x138FC3B6u, 0x138FC3B6u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Pistol_Eon_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Eon_Equip_Up_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Pistol_Eon_Equip_Up_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Pistol_Eon_Equip_Up_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xC4088A91u, 0xC4088A91u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Pistol_Eon_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Eon_Equip_Down_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Pistol_Eon_Equip_Down_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Pistol_Eon_Equip_Down_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
            } },
        WeaponSpeakerProfile{
            "Va'ruun Starshard",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xD88A351Cu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_Inflictor_PC_Fire_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Inflictor_PC_Fire_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_Inflictor_PC_Fire_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Pistol_Inflictor_PC_Fire_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Pistol_Inflictor_PC_Fire_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xA9821DB7u, 0xA9821DB7u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Inflictor_Reload_MagOut_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Inflictor_Reload_MagOut_FirstPerson_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "clip-grab", WeaponSpeakerTrigger::WwisePost,
                    0xF41DEDD8u, 0xF41DEDD8u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Inflictor_Reload_ClipGrab_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Inflictor_Reload_ClipGrab_FirstPerson_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x811F6050u, 0x811F6050u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Inflictor_Reload_MagIn_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Inflictor_Reload_MagIn_FirstPerson_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xF24ED822u, 0xF24ED822u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Inflictor_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Inflictor_Equip_Up_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x695EA915u, 0x695EA915u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Inflictor_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Inflictor_Equip_Down_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
            } },
        WeaponSpeakerProfile{
            "Va'ruun Inflictor",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x3F317374u, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Inflictor_PC_Fire_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Inflictor_PC_Fire_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Inflictor_PC_Fire_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_Inflictor_PC_Fire_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_Inflictor_PC_Fire_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xE65B1142u, 0xE65B1142u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Rifle_Inflictor_Reload_MagOut_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Inflictor_Reload_MagOut_FirstPerson_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Inflictor_Reload_MagOut_FirstPerson_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "sparks", WeaponSpeakerTrigger::WwisePost,
                    0x490A06ABu, 0x490A06ABu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Rifle_Inflictor_Reload_Sparks_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Inflictor_Reload_Sparks_FirstPerson_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Inflictor_Reload_Sparks_FirstPerson_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x96EDD9DFu, 0x96EDD9DFu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Rifle_Inflictor_Reload_MagIn_FirstPerson_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Inflictor_Reload_MagIn_FirstPerson_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Inflictor_Reload_MagIn_FirstPerson_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x8734B3B1u, 0x8734B3B1u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Rifle_Inflictor_Equip_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Inflictor_Equip_Up_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Inflictor_Equip_Up_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x346995B2u, 0x346995B2u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Rifle_Inflictor_Equip_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Inflictor_Equip_Down_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Inflictor_Equip_Down_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
            } },
        WeaponSpeakerProfile{
            "Novablast Disruptor",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xBC13C42Du, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, " WPN_Hand_Pistol_Novablast_Fire_PC_Suppressor_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, " WPN_Hand_Pistol_Novablast_Fire_PC_Suppressor_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, " WPN_Hand_Pistol_Novablast_Fire_PC_Suppressor_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, " WPN_Hand_Pistol_Novablast_Fire_PC_Suppressor_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out-a", WeaponSpeakerTrigger::WwisePost,
                    0x9CB6F749u, 0x9CB6F749u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Pistol_Novablast_Reload_MagOut_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Novablast_Reload_MagOut_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-up", WeaponSpeakerTrigger::WwisePost,
                    0x80528B07u, 0x80528B07u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Novablast_Reload_MagUp_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out-b", WeaponSpeakerTrigger::WwisePost,
                    0x000A65B4u, 0x000A65B4u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Pistol_Novablast_Reload_MagOut_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Novablast_Reload_MagOut_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x9CB6F74Au, 0x9CB6F74Au, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Pistol_Novablast_Reload_MagIn_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Novablast_Reload_MagIn_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "cover-open", WeaponSpeakerTrigger::WwisePost,
                    0x072FF244u, 0x072FF244u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Novablast_Reload_Cover_Open_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "cover-close", WeaponSpeakerTrigger::WwisePost,
                    0x48E46030u, 0x48E46030u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Novablast_Reload_Cover_Close_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "bolt-close", WeaponSpeakerTrigger::WwisePost,
                    0x0F9455F2u, 0x0F9455F2u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Pistol_Novablast_Reload_BoltClose_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Pistol_Novablast_Reload_BoltClose_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x7D5840F7u, 0x7D5840F7u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Equip_Up_02.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x3471125Cu, 0x3471125Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Pistol_Eon_Equip_Down_02.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            } },
        WeaponSpeakerProfile{
            "Va'ruun Quickstrike",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xDA2C534Au, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 6u, "WPN_Hand_Pistol_Solstice_Fire_PC_Semi_V2_06.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xCD519327u, 0xCD519327u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Solstice_Reload_MagOut_V2_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Solstice_Reload_MagOut_V2_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-grab", WeaponSpeakerTrigger::WwisePost,
                    0x9240A702u, 0x9240A702u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Solstice_Reload_MagOut_pt2_V2_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Solstice_Reload_MagOut_pt2_V2_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xF9D4D3DAu, 0xF9D4D3DAu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Solstice_Reload_MagIn_V2_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Solstice_Reload_MagIn_V2_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x28FC1D9Eu, 0x28FC1D9Eu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Combatech_Solstice_Equip_Up_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Combatech_Solstice_Equip_Up_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x56D49E79u, 0x56D49E79u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Hand_Pistol_Combatech_Solstice_Equip_Down_PC_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Pistol_Combatech_Solstice_Equip_Down_PC_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
            }, "Solstice" },
        WeaponSpeakerProfile{
            "Va'ruun Longfang",
            {
                WeaponSpeakerCue{
                    "fire", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0x926F52DAu, 0u, 0u, false, kBatchGain, kFireMaxFrames, kFireFadeFrames,
                    {
                        { 1u, "WPN_Hand_Rifle_Orion_Fire_v8_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Hand_Rifle_Orion_Fire_v8_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 3u, "WPN_Hand_Rifle_Orion_Fire_v8_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 4u, "WPN_Hand_Rifle_Orion_Fire_v8_04.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 5u, "WPN_Hand_Rifle_Orion_Fire_v8_05.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0xA52BB1D3u, 0xA52BB1D3u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Reload_1_MagOut_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x262CFE9Bu, 0x262CFE9Bu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Reload_2_MagIn_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "raise", WeaponSpeakerTrigger::WwisePost,
                    0xFD260E36u, 0xFD260E36u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Reload_3_RaiseGun_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xF68897F7u, 0xF68897F7u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Equip_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xA0BAC15Cu, 0xA0BAC15Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Hand_Rifle_Orion_Unequip_v3.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
            }, "Orion" },
        WeaponSpeakerProfile{
            "Va'ruun Penumbra",
            {
                WeaponSpeakerCue{
                    "fire-body", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xF96C72FFu, 0u, 0u, false, kBatchGain, 0u, 0u,
                    {
                        { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 181682584u },
                        { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 284040293u },
                        { 3u, "", WeaponSpeakerArchivePolicy::PreferPatch, 285796894u },
                        { 4u, "", WeaponSpeakerArchivePolicy::PreferPatch, 305279828u },
                        { 5u, "", WeaponSpeakerArchivePolicy::PreferPatch, 430044306u },
                        { 6u, "", WeaponSpeakerArchivePolicy::PreferPatch, 711836228u },
                        { 7u, "", WeaponSpeakerArchivePolicy::PreferPatch, 1003579949u },
                        { 8u, "", WeaponSpeakerArchivePolicy::PreferPatch, 1052084031u },
                    } },
                WeaponSpeakerCue{
                    "fire-low", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xF96C72FFu, 0u, 0u, false, kBatchGain, 0u, 0u,
                    {
                        { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 272400732u },
                        { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 570040591u },
                        { 3u, "", WeaponSpeakerArchivePolicy::PreferPatch, 787639278u },
                        { 4u, "", WeaponSpeakerArchivePolicy::PreferPatch, 790373385u },
                    } },
                WeaponSpeakerCue{
                    "fire-high", WeaponSpeakerTrigger::ConfirmedWeaponFire,
                    0xF96C72FFu, 0u, 0u, false, kBatchGain, 0u, 0u,
                    {
                        { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 10032930u },
                        { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 471580376u },
                        { 3u, "", WeaponSpeakerArchivePolicy::PreferPatch, 672798812u },
                        { 4u, "", WeaponSpeakerArchivePolicy::PreferPatch, 804813834u },
                    } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x09BBD2D5u, 0x09BBD2D5u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 26369956u },
                        { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 114800266u },
                        { 3u, "", WeaponSpeakerArchivePolicy::PreferPatch, 289065960u },
                        { 4u, "", WeaponSpeakerArchivePolicy::PreferPatch, 335322172u },
                        { 5u, "", WeaponSpeakerArchivePolicy::PreferPatch, 560023525u },
                        { 6u, "", WeaponSpeakerArchivePolicy::PreferPatch, 649426097u },
                    } },
                WeaponSpeakerCue{ "bolt-open", WeaponSpeakerTrigger::WwisePost,
                    0x25F00FC7u, 0x25F00FC7u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 569664042u },
                        { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 899772201u },
                        { 3u, "", WeaponSpeakerArchivePolicy::PreferPatch, 938596555u },
                    } },
                WeaponSpeakerCue{ "bullet-handling", WeaponSpeakerTrigger::WwisePost,
                    0xB08A20CCu, 0xB08A20CCu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 46163035u },
                        { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 358054682u },
                        { 3u, "", WeaponSpeakerArchivePolicy::PreferPatch, 1055451401u },
                    } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x2E403A7Cu, 0x2E403A7Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 6753058u },
                        { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 44588606u },
                        { 3u, "", WeaponSpeakerArchivePolicy::PreferPatch, 766176397u },
                    } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x40FB1CC4u, 0x40FB1CC4u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 277384954u },
                        { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 294831366u },
                        { 3u, "", WeaponSpeakerArchivePolicy::PreferPatch, 437483911u },
                    } },
            } },
        WeaponSpeakerProfile{
            "Va'ruun Starstorm",
            {
                WeaponSpeakerCue{ "power-down", WeaponSpeakerTrigger::WwisePost,
                    0xFB7756F7u, 0xFB7756F7u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 920864464u } } },
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x0444A7EAu, 0x0444A7EAu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 102768015u } } },
                WeaponSpeakerCue{ "push-lever", WeaponSpeakerTrigger::WwisePost,
                    0x1BFBE240u, 0x1BFBE240u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 652288839u } } },
                WeaponSpeakerCue{ "remove-mag", WeaponSpeakerTrigger::WwisePost,
                    0x00465690u, 0x00465690u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 356738806u } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0xD957DC9Au, 0xD957DC9Au, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 887977508u } } },
                WeaponSpeakerCue{ "bolt-back", WeaponSpeakerTrigger::WwisePost,
                    0xAFD57B91u, 0xAFD57B91u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 312734531u },
                        { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 871471746u },
                    } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0xCCFD7993u, 0xCCFD7993u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 530763176u } } },
            }, "", WeaponSpeakerSustainedCue{
                0xB6E1A82Eu, 0xA7514E2Du, 0x2u, true, kStarstormSustainedBodyGain,
                {
                    { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 963157375u },
                },
                {
                    { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 148515386u },
                    { 2u, "", WeaponSpeakerArchivePolicy::PreferPatch, 260127898u },
                    { 3u, "", WeaponSpeakerArchivePolicy::PreferPatch, 502172401u },
                    { 4u, "", WeaponSpeakerArchivePolicy::PreferPatch, 510628109u },
                    { 5u, "", WeaponSpeakerArchivePolicy::PreferPatch, 768352595u },
                    { 6u, "", WeaponSpeakerArchivePolicy::PreferPatch, 805444445u },
                    { 7u, "", WeaponSpeakerArchivePolicy::PreferPatch, 874666534u },
                },
                {
                    { 1u, "", WeaponSpeakerArchivePolicy::PreferPatch, 480391941u },
                },
                true, kBatchGain, kBatchGain } },
        WeaponSpeakerProfile{
            "Arc Welder",
            {
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0xFB8B0332u, 0xFB8B0332u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Arc_Welder_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Arc_Welder_Up_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "reload-start", WeaponSpeakerTrigger::WwisePost,
                    0x664B7DC8u, 0x664B7DC8u, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Arc_Welder_Reload_01_Start_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-out", WeaponSpeakerTrigger::WwisePost,
                    0x228E871Bu, 0x228E871Bu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Arc_Welder_Reload_02_Mag_Out_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "mag-in", WeaponSpeakerTrigger::WwisePost,
                    0x95DA6FDFu, 0x95DA6FDFu, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Arc_Welder_Reload_03_Mag_In_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "reload-end", WeaponSpeakerTrigger::WwisePost,
                    0x8D1E286Au, 0x8D1E286Au, 0x2u, true, kBatchGain, 0u, 0u,
                    { { 1u, "WPN_Arc_Welder_Reload_04_End_01.wav", WeaponSpeakerArchivePolicy::PreferPatch } } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x97EC41C5u, 0x97EC41C5u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Arc_Welder_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Arc_Welder_Down_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
            }, "", WeaponSpeakerSustainedCue{
                0xBB87C268u, 0x46C5B7DAu, 0x2u, true, kBatchGain,
                {
                    { 1u, "WPN_Arc_Welder_Fire_02_Player_01_LP_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 2u, "WPN_Arc_Welder_Fire_02_Player_02_LP_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 3u, "WPN_Arc_Welder_Fire_02_Player_03_LP_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                },
                {
                    { 1u, "WPN_Arc_Welder_Fire_01_Trigger_Press_Player_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 2u, "WPN_Arc_Welder_Fire_01_Trigger_Press_Player_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 3u, "WPN_Arc_Welder_Fire_01_Trigger_Press_Player_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                },
                {
                    { 1u, "WPN_Arc_Welder_Fire_03_Trigger_Release_Player_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 2u, "WPN_Arc_Welder_Fire_03_Trigger_Release_Player_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 3u, "WPN_Arc_Welder_Fire_03_Trigger_Release_Player_03.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                }, false, kBatchGain, kBatchGain } },
        WeaponSpeakerProfile{
            "Cutter",
            {
                WeaponSpeakerCue{ "draw", WeaponSpeakerTrigger::WwisePost,
                    0x1EA42CA7u, 0x1EA42CA7u, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Cutter_Up_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Cutter_Up_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
                WeaponSpeakerCue{ "holster", WeaponSpeakerTrigger::WwisePost,
                    0x000B852Cu, 0x000B852Cu, 0x2u, true, kBatchGain, 0u, 0u,
                    {
                        { 1u, "WPN_Cutter_Down_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                        { 2u, "WPN_Cutter_Down_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    } },
            }, "", WeaponSpeakerSustainedCue{
                0x8EDCB1C7u, 0x40FF9B15u, 0x2u, true, kBatchGain,
                {
                    { 1u, "WPN_Cutter_Fire_Player_01_LP_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 2u, "WPN_Cutter_Fire_Player_02_LP_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 3u, "WPN_Cutter_Fire_Player_03_LP_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                },
                {
                    { 1u, "WPN_Cutter_Fire_Trigger_Press_Player_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 2u, "WPN_Cutter_Fire_Trigger_Press_Player_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                },
                {
                    { 1u, "WPN_Cutter_Fire_Trigger_Release_Player_01.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                    { 2u, "WPN_Cutter_Fire_Trigger_Release_Player_02.wav", WeaponSpeakerArchivePolicy::PreferPatch },
                }, false, kBatchGain, kBatchGain } },
    };
}

std::span<const sds::WeaponSpeakerProfile> sds::weaponSpeakerProfiles() noexcept
{
    return kProfiles;
}

const sds::WeaponSpeakerProfile* sds::findWeaponSpeakerProfile(std::string_view weaponIdentity) noexcept
{
    for (const auto& profile : kProfiles) {
        if (profile.weaponIdentity == weaponIdentity) {
            return &profile;
        }
    }
    return nullptr;
}


const sds::WeaponSpeakerProfile* sds::findWeaponSpeakerAudioFamilyProfile(
    std::string_view logicalWeapon) noexcept
{
    const auto* logical = findWeaponSpeakerProfile(logicalWeapon);
    if (!logical) {
        return nullptr;
    }
    return findWeaponSpeakerProfile(speakerAudioFamily(*logical));
}

std::size_t sds::weaponSpeakerAudioFamilyCount() noexcept
{
    std::size_t count = 0u;
    for (std::size_t i = 0; i < kProfiles.size(); ++i) {
        const auto family = speakerAudioFamily(kProfiles[i]);
        bool seen = false;
        for (std::size_t j = 0; j < i; ++j) {
            if (speakerAudioFamily(kProfiles[j]) == family) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            ++count;
        }
    }
    return count;
}

std::size_t sds::weaponSpeakerPhysicalVariantCount() noexcept
{
    std::size_t count = 0u;
    for (std::size_t i = 0; i < kProfiles.size(); ++i) {
        const auto family = speakerAudioFamily(kProfiles[i]);
        bool seen = false;
        for (std::size_t j = 0; j < i; ++j) {
            if (speakerAudioFamily(kProfiles[j]) == family) {
                seen = true;
                break;
            }
        }
        if (seen) {
            continue;
        }
        const auto* canonical = findWeaponSpeakerProfile(family);
        if (!canonical) {
            continue;
        }
        for (const auto& cue : canonical->cues) {
            count += cue.variants.size();
        }
        if (canonical->sustained) {
            count += canonical->sustained->loopVariants.size();
            count += canonical->sustained->startTransientVariants.size();
            count += canonical->sustained->stopTransientVariants.size();
        }
    }
    return count;
}

const sds::WeaponSpeakerCue* sds::findWeaponSpeakerCue(
    const WeaponSpeakerProfile& profile,
    std::string_view action) noexcept
{
    for (const auto& cue : profile.cues) {
        if (cue.action == action) {
            return &cue;
        }
    }
    return nullptr;
}

const sds::WeaponSpeakerSustainedCue* sds::findWeaponSpeakerSustainedCue(
    const WeaponSpeakerProfile& profile) noexcept
{
    return profile.sustained ? &*profile.sustained : nullptr;
}
