/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_ENCODER_H
#define RADEON_HD_ENCODER_H


void encoder_assign_crtc(uint8 crt_id);
void encoder_apply_quirks(uint8 crtcID);
void encoder_mode_set(uint8 id, uint32 pixelClock);
status_t encoder_digital_setup(uint8 id, uint32 pixelClock, int command);
status_t encoder_analog_setup(uint8 id, uint32 pixelClock, int command);
status_t encoder_dig_setup(uint8 id, uint32 pixelClock, int command);
status_t encoder_tv_setup(uint8 id, uint32 pixelClock, int command);
bool encoder_analog_load_detect(uint8 connectorIndex);
void encoder_output_lock(bool lock);
void encoder_crtc_scratch(uint8 crtcID);
void encoder_dpms_scratch(uint8 crtcID, bool power);
void encoder_dpms_set(uint8 crtcID, uint8 encoderID, int mode);


#endif /* RADEON_HD_ENCODER_H */
