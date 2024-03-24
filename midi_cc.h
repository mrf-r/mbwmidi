#ifndef _MIDI_CC_H
#define _MIDI_CC_H

typedef enum {
    MIDI_CC_00_BANKSELECT_MSB = 0x00,
    MIDI_CC_01_MODWHEEL_MSB = 0x01,
    MIDI_CC_02_BREATH_MSB = 0x02,
    MIDI_CC_03_UNDEF_MSB = 0x03,
    MIDI_CC_04_FOOTPEDAL_MSB = 0x04,
    MIDI_CC_05_PORTATIME_MSB = 0x05,
    MIDI_CC_06_DATAENTRY_MSB = 0x06,
    MIDI_CC_07_VOLUME_MSB = 0x07,
    MIDI_CC_08_BALANCE_MSB = 0x08,
    MIDI_CC_09_UNDEF_MSB = 0x09,
    MIDI_CC_0A_PAN_MSB = 0x0A,
    MIDI_CC_0B_EXPRESSION_MSB = 0x0B,
    MIDI_CC_0C_EFFECT1_MSB = 0x0C,
    MIDI_CC_0D_EFFECT2_MSB = 0x0D,
    MIDI_CC_0E_UNDEF_MSB = 0x0E,
    MIDI_CC_0F_UNDEF_MSB = 0x0F,

    MIDI_CC_10_GP_MSB = 0x10,
    MIDI_CC_11_GP_MSB = 0x11,
    MIDI_CC_12_GP_MSB = 0x12,
    MIDI_CC_13_GP_MSB = 0x13,
    MIDI_CC_14_UNDEF_MSB = 0x14,
    MIDI_CC_15_UNDEF_MSB = 0x15,
    MIDI_CC_16_UNDEF_MSB = 0x16,
    MIDI_CC_17_UNDEF_MSB = 0x17,
    MIDI_CC_18_UNDEF_MSB = 0x18,
    MIDI_CC_19_UNDEF_MSB = 0x19,
    MIDI_CC_1A_UNDEF_MSB = 0x1A,
    MIDI_CC_1B_UNDEF_MSB = 0x1B,
    MIDI_CC_1C_UNDEF_MSB = 0x1C,
    MIDI_CC_1D_UNDEF_MSB = 0x1D,
    MIDI_CC_1E_UNDEF_MSB = 0x1E,
    MIDI_CC_1F_UNDEF_MSB = 0x1F,

    MIDI_CC_20_BANKSELECT_LSB = 0x20,
    MIDI_CC_21_MODWHEEL_LSB = 0x21,
    MIDI_CC_22_BREATH_LSB = 0x22,
    MIDI_CC_23_UNDEF_LSB = 0x23,
    MIDI_CC_24_FOOTPEDAL_LSB = 0x24,
    MIDI_CC_25_PORTATIME_LSB = 0x25,
    MIDI_CC_26_DATAENTRY_LSB = 0x26,
    MIDI_CC_27_VOLUME_LSB = 0x27,
    MIDI_CC_28_BALANCE_LSB = 0x28,
    MIDI_CC_29_UNDEF_LSB = 0x29,
    MIDI_CC_2A_PAN_LSB = 0x2A,
    MIDI_CC_2B_EXPRESSION_LSB = 0x2B,
    MIDI_CC_2C_EFFECT1_LSB = 0x2C,
    MIDI_CC_2D_EFFECT2_LSB = 0x2D,
    MIDI_CC_2E_UNDEF_LSB = 0x2E,
    MIDI_CC_2F_UNDEF_LSB = 0x2F,

    MIDI_CC_30_GP_LSB = 0x30,
    MIDI_CC_31_GP_LSB = 0x31,
    MIDI_CC_32_GP_LSB = 0x32,
    MIDI_CC_33_GP_LSB = 0x33,
    MIDI_CC_34_UNDEF_LSB = 0x34,
    MIDI_CC_35_UNDEF_LSB = 0x35,
    MIDI_CC_36_UNDEF_LSB = 0x36,
    MIDI_CC_37_UNDEF_LSB = 0x37,
    MIDI_CC_38_UNDEF_LSB = 0x38,
    MIDI_CC_39_UNDEF_LSB = 0x39,
    MIDI_CC_3A_UNDEF_LSB = 0x3A,
    MIDI_CC_3B_UNDEF_LSB = 0x3B,
    MIDI_CC_3C_UNDEF_LSB = 0x3C,
    MIDI_CC_3D_UNDEF_LSB = 0x3D,
    MIDI_CC_3E_UNDEF_LSB = 0x3E,
    MIDI_CC_3F_UNDEF_LSB = 0x3F,

    MIDI_CC_40_DAMPERPEDAL_SW = 0x40, // switch - on/off
    MIDI_CC_41_PORTAMENTO_SW = 0x41,
    MIDI_CC_42_SOSTENUTO_SW = 0x42,
    MIDI_CC_43_SOFTPEDAL_SW = 0x43,
    MIDI_CC_44_LEGATOFOOT_SW = 0x44,
    MIDI_CC_45_HOLD2_SW = 0x45,
    MIDI_CC_46_SOUNDCTRL1 = 0x46,
    MIDI_CC_47_SOUNDCTRL2 = 0x47,
    MIDI_CC_48_SOUNDCTRL3 = 0x48,
    MIDI_CC_49_SOUNDCTRL4 = 0x49,
    MIDI_CC_4A_SOUNDCTRL5 = 0x4A,
    MIDI_CC_4B_SOUNDCTRL6 = 0x4B,
    MIDI_CC_4C_SOUNDCTRL7 = 0x4C,
    MIDI_CC_4D_SOUNDCTRL8 = 0x4D,
    MIDI_CC_4E_SOUNDCTRL9 = 0x4E,
    MIDI_CC_4F_SOUNDCTRL10 = 0x4F,

    MIDI_CC_50_GP = 0x50,
    MIDI_CC_51_GP = 0x51,
    MIDI_CC_52_GP = 0x52,
    MIDI_CC_53_GP = 0x53,
    MIDI_CC_54_PORTAMENTO = 0x54,
    MIDI_CC_55_UNDEF = 0x55,
    MIDI_CC_56_UNDEF = 0x56,
    MIDI_CC_57_UNDEF = 0x57,
    MIDI_CC_58_HIRESVELO_PFX = 0x58,
    MIDI_CC_59_UNDEF = 0x59,
    MIDI_CC_5A_UNDEF = 0x5A,
    MIDI_CC_5B_FX1DEPTH = 0x5B,
    MIDI_CC_5C_FX2DEPTH = 0x5C,
    MIDI_CC_5D_FX3DEPTH = 0x5D,
    MIDI_CC_5E_FX4DEPTH = 0x5E,
    MIDI_CC_5F_FX5DEPTH = 0x5F,

    MIDI_CC_60_DATAINC_ND = 0x60, // _ND - data ignored (by MIDI specs)
    MIDI_CC_61_DATADEC_ND = 0x61,
    MIDI_CC_62_NRPN_LSB = 0x62,
    MIDI_CC_63_NRPN_MSB = 0x63,
    MIDI_CC_64_RPN_LSB = 0x64,
    MIDI_CC_65_RPN_MSB = 0x65,
    MIDI_CC_66_UNDEF = 0x66,
    MIDI_CC_67_UNDEF = 0x67,
    MIDI_CC_68_UNDEF = 0x68,
    MIDI_CC_69_UNDEF = 0x69,
    MIDI_CC_6A_UNDEF = 0x6A,
    MIDI_CC_6B_UNDEF = 0x6B,
    MIDI_CC_6C_UNDEF = 0x6C,
    MIDI_CC_6D_UNDEF = 0x6D,
    MIDI_CC_6E_UNDEF = 0x6E,
    MIDI_CC_6F_UNDEF = 0x6F,

    MIDI_CC_70_UNDEF = 0x70,
    MIDI_CC_71_UNDEF = 0x71,
    MIDI_CC_72_UNDEF = 0x72,
    MIDI_CC_73_UNDEF = 0x73,
    MIDI_CC_74_UNDEF = 0x74,
    MIDI_CC_75_UNDEF = 0x75,
    MIDI_CC_76_UNDEF = 0x76,
    MIDI_CC_77_UNDEF = 0x77,
    MIDI_CC_78_ALLSOUNDOFF_ND = 0x78,
    MIDI_CC_79_RESETALLCONTROLLERS_ND = 0x79,
    MIDI_CC_7A_LOCALONOFF_SW = 0x7A,
    MIDI_CC_7B_ALLNOTESOFF_ND = 0x7B,
    MIDI_CC_7C_OMNIOFF_ND = 0x7C,
    MIDI_CC_7D_OMNION_ND = 0x7D,
    MIDI_CC_7E_MONOPHONIC_ND = 0x7E,
    MIDI_CC_7F_POLYPHONIC_ND = 0x7F,
} MidiCCEn;

#endif // _MIDI_CC_H