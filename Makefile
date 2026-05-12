.PHONY: build run run-bg stop test clean

BUILD_DIR ?= cmake-build-debug
CMAKE     ?= /Users/felipe/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake
PKG_PATH   = /opt/homebrew/lib/pkgconfig

build:
	PKG_CONFIG_PATH="$(PKG_PATH)" $(CMAKE) -B $(BUILD_DIR)
	$(CMAKE) --build $(BUILD_DIR)

run:
	./$(BUILD_DIR)/rinha_de_backend

run-bg:
	./$(BUILD_DIR)/rinha_de_backend &

stop:
	pkill -f rinha_de_backend 2>/dev/null; true

test: build
	$(MAKE) stop
	$(MAKE) run-bg
	sleep 0.5
	curl -s http://localhost:8081/ready
	@echo ""
	curl -s -X POST http://localhost:8081/fraud-score \
		-H "Content-Type: application/json" \
		-d '{"id":"tx-1","transaction":{"amount":100.50,"installments":1,"requested_at":"2026-05-11T12:00:00Z"},"customer":{"avg_amount":200.0,"tx_count_24h":5,"known_merchants":["m1"]},"merchant":{"id":"m1","mcc":"5411","avg_amount":120.0},"terminal":{"is_online":true,"card_present":false,"km_from_home":10.0}}'
	@echo ""
	curl -s http://localhost:8081/ || true
	@echo ""
	$(MAKE) stop

clean:
	rm -rf $(BUILD_DIR)
