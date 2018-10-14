include mk/Makefile.kernel

app:
	@echo "[ACTION] start to compile application packages."; \
	for _dir in `ls application`; \
	do \
		ZELDA=$(ZELDA) make -C application/$$_dir; \
	done

app_install:
	@echo "[ACTION] start to install application packages."; \
	for _dir in `ls application`; \
	do \
		ZELDA=$(ZELDA) make install -C application/$$_dir; \
	done
app_clean:
	@echo "[ACTION] start to clean application packages."; \
	for _dir in `ls application`; \
	do \
		ZELDA=$(ZELDA) make clean -C application/$$_dir; \
	done

runtime_install:
	@echo "[ACTION] start to compile native C lib and runtime."
	@ZELDA=$(ZELDA) make -C runtime/

runtime_clean:
	@echo "[ACTION] start to clean native C lib and runtime."
	@ZELDA=$(ZELDA) make clean -C runtime/
