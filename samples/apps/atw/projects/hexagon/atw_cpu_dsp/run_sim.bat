@echo The Hexagon SDK must be installed to C:\Qualcomm\Hexagon_SDK\3.0\
@echo Run:
@echo     C:\Qualcomm\Hexagon_SDK\3.0\setup_sdk_env.cmd
@echo Specify "hotcache" for hot cache cycle counts.

call make tree V_toolv72=1 V_ARCH=v60 V=hexagon_ReleaseG
@if "%1" == "hotcache" (
	@echo Measuring cycles with hot cache.
	C:\Qualcomm\HEXAGON_Tools\7.2.12\Tools\bin\hexagon-sim -mv60 --simulated_returnval --usefs hexagon_ReleaseG --pmu_statsfile hexagon_ReleaseG/pmu_stats.txt hexagon_ReleaseG/TimeWarp_q --rtos hexagon_ReleaseG/osam.cfg --symfile hexagon_ReleaseG/TimeWarp_q --cosim_file hexagon_ReleaseG/q6ss.cfg --dsp_clock 800
) else (
	@echo Measuring cycles with 40 cycle memory latency.
	C:\Qualcomm\HEXAGON_Tools\7.2.12\Tools\bin\hexagon-sim -mv60 --simulated_returnval --usefs hexagon_ReleaseG --pmu_statsfile hexagon_ReleaseG/pmu_stats.txt hexagon_ReleaseG/TimeWarp_q --rtos hexagon_ReleaseG/osam.cfg --symfile hexagon_ReleaseG/TimeWarp_q --cosim_file hexagon_ReleaseG/q6ss.cfg --dsp_clock 800 --timing --buspenalty 40
)
