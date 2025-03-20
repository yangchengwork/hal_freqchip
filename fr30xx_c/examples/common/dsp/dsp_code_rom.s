    AREA RO,DATA,READONLY 

; dsp_code_flash is compiled based on ROM code is stored in IROM
    EXPORT DSP_CODE_ROM_BASE
    EXPORT DSP_CODE_ROM_END
DSP_CODE_ROM_BASE
        incbin dsp_code_rom
DSP_CODE_ROM_END

    END
