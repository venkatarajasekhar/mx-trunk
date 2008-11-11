#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_poison.h"

#include "mxconfig.h"
#include "mx_osdef.h"
#include "mx_stdint.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_image.h"
#include "mx_math.h"

#include "mx_amplifier.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_area_detector.h"
#include "mx_array.h"
#include "mx_ascii.h"
#include "mx_autoscale.h"
#include "mx_bit.h"
#include "mx_bluice.h"
#include "mx_callback.h"
#include "mx_camac.h"
#include "mx_camera_link.h"
#include "mx_camera_link_api.h"
#include "mx_cfn.h"
#include "mx_cfn_defaults.h"
#include "mx_clock.h"
#include "mx_constants.h"
#include "mx_coprocess.h"
#include "mx_datafile.h"
#include "mx_dead_reckoning.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_dirent.h"
#include "mx_dynamic_library.h"
#include "mx_encoder.h"
#include "mx_epics.h"
#include "mx_generic.h"
#include "mx_gpib.h"
#include "mx_handle.h"
#include "mx_hrt.h"
#include "mx_hrt_debug.h"
#include "mx_interval_timer.h"
#include "mx_inttypes.h"
#include "mx_key.h"
#include "mx_list.h"
#include "mx_list_head.h"
#include "mx_log.h"
#include "mx_mca.h"
#include "mx_mcai.h"
#include "mx_mce.h"
#include "mx_mcs.h"
#include "mx_measurement.h"
#include "mx_memory.h"
#include "mx_mfault.h"
#include "mx_modbus.h"
#include "mx_motor.h"
#include "mx_mpermit.h"
#include "mx_mutex.h"
#include "mx_namefix.h"
#include "mx_net.h"
#include "mx_net_socket.h"
#include "mx_pipe.h"
#include "mx_plot.h"
#include "mx_portio.h"
#include "mx_private_limits.h"
#include "mx_process.h"
#include "mx_program_model.h"
#include "mx_ptz.h"
#include "mx_pulse_generator.h"
#include "mx_record.h"
#include "mx_relay.h"
#include "mx_rs232.h"
#include "mx_sample_changer.h"
#include "mx_sca.h"
#include "mx_scaler.h"
#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "mx_scan_list.h"
#include "mx_scan_quick.h"
#include "mx_scan_xafs.h"
#include "mx_select.h"
#include "mx_semaphore.h"
#include "mx_signal.h"
#include "mx_socket.h"
#include "mx_spec.h"
#include "mx_syslog.h"
#include "mx_table.h"
#include "mx_test.h"
#include "mx_thread.h"
#include "mx_timer.h"
#include "mx_unistd.h"
#include "mx_usb.h"
#include "mx_variable.h"
#include "mx_vepics.h"
#include "mx_version.h"
#include "mx_vfile.h"
#include "mx_video_input.h"
#include "mx_vinline.h"
#include "mx_virtual_timer.h"
#include "mx_vme.h"
#include "mx_vnet.h"
#include "mx_xdr.h"

int
main( int argc, char *argv[] )
{
	printf( "C++ include file test succeeded!\n" );

	exit(0);
}

