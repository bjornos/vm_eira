# All Target
all: vm_eira

# Tool invocations
vm_eira: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Linker'
	gcc  -o "vm_eira" main.c
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) vm_eira
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
