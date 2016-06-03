#always first
include glue/defines.min

# include the variant specific .min file.
# V=hexagon --> hexagon.min
# V=WinNT --> WinNT.min
# V=android --> android.min
ifneq (,$(findstring hexagon,$(V_TARGET)))
  include hexagon.min
else
  include $(V_TARGET).min
endif

#always last
include $(RULES_MIN)
