    AREA RO_REVB,DATA,READONLY 

    EXPORT CONTROLLER_CODE_REVB_BASE
    EXPORT CONTROLLER_CODE_REVB_END

CONTROLLER_CODE_REVB_BASE
        incbin controller_code_revb
CONTROLLER_CODE_REVB_END

    AREA RO_REVC,DATA,READONLY 
    EXPORT CONTROLLER_CODE_REVC_BASE
    EXPORT CONTROLLER_CODE_REVC_END

CONTROLLER_CODE_REVC_BASE
        incbin controller_code_revc
CONTROLLER_CODE_REVC_END

    END
