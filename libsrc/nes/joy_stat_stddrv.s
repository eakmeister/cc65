;
; Address of the static standard joystick driver
;
; Oliver Schmidt, 2012-11-01
;
; const void joy_static_stddrv[];
;

        .export	_joy_static_stddrv
        .import	_nes_stdjoy

.rodata

_joy_static_stddrv := _nes_stdjoy
