export BUILDTYPE ?= Debug
export IS_LOCAL_DEVELOPMENT ?= true
export TARGET_BRANCH ?= main
export CI ?= 0
export DEV_TEAM ?= ""

# When only the dependencies in the package.json are installed
# Node.js will error when running a script from the root scripts directory
# Getting rid of the duplicate package.json file is an open issue
# https://github.com/maplibre/maplibre-native/issues/657

export NODE_PATH = ../../node_modules

CMAKE ?= cmake


ifeq ($(BUILDTYPE), Release)
else ifeq ($(BUILDTYPE), RelWithDebInfo)
else ifeq ($(BUILDTYPE), Sanitize)
else ifeq ($(BUILDTYPE), Debug)
else
  $(error BUILDTYPE must be Debug, Sanitize, Release or RelWithDebInfo)
endif

buildtype := $(shell echo "$(BUILDTYPE)" | tr "[A-Z]" "[a-z]")

ifeq ($(shell uname -s), Darwin)
  HOST_PLATFORM = macos
  HOST_PLATFORM_VERSION = $(shell uname -m)
  export NINJA = platform/macos/ninja
  export NCPU := $(shell sysctl -n hw.ncpu)
  export JOBS ?= $(shell expr $(NCPU) - 1)
else ifeq ($(shell uname -s), Linux)
  HOST_PLATFORM = linux
  HOST_PLATFORM_VERSION = $(shell uname -m)
  export NINJA = platform/linux/ninja
  export JOBS ?= $(shell grep --count processor /proc/cpuinfo)
else
  $(error Cannot determine host platform)
endif

ifeq ($(MASON_PLATFORM),)
  BUILD_PLATFORM = $(HOST_PLATFORM)
else
  BUILD_PLATFORM = $(MASON_PLATFORM)
endif

ifeq ($(MASON_PLATFORM_VERSION),)
  BUILD_PLATFORM_VERSION = $(HOST_PLATFORM_VERSION)
else
  BUILD_PLATFORM_VERSION = $(MASON_PLATFORM_VERSION)
endif

ifeq ($(MASON_PLATFORM),macos)
	MASON_PLATFORM=osx
endif

ifeq ($(V), 1)
  export XCPRETTY
  NINJA_ARGS ?= -v
else
  export XCPRETTY ?= | tee '$(shell pwd)/build/xcodebuild-$(shell date +"%Y-%m-%d_%H%M%S").log' | xcpretty
  NINJA_ARGS ?=
endif

.PHONY: default
default: test

BUILD_DEPS += ../../CMakeLists.txt

BUILD_DOCS ?= true

#### iOS targets ##############################################################

ifeq ($(HOST_PLATFORM), macos)

IOS_OUTPUT_PATH = build/ios
IOS_PROJ_PATH = $(IOS_OUTPUT_PATH)/Mapbox\ GL\ Native.xcodeproj
IOS_WORK_PATH = platform/ios/ios.xcworkspace
IOS_USER_DATA_PATH = $(IOS_WORK_PATH)/xcuserdata/$(USER).xcuserdatad

IOS_XCODEBUILD_SIM = xcodebuild \
	ARCHS=arm64 ONLY_ACTIVE_ARCH=YES \
	-derivedDataPath $(IOS_OUTPUT_PATH) \
	-configuration $(BUILDTYPE) -sdk iphonesimulator \
	-workspace $(IOS_WORK_PATH) \
	-jobs $(JOBS)

ifneq ($(MORE_SIMULATORS),)
	IOS_LATEST = true
endif

ifdef IOS_LATEST
	IOS_XCODEBUILD_SIM += \
	-destination 'platform=iOS Simulator,OS=latest,name=iPhone 14 Pro' \
	-destination 'platform=iOS Simulator,OS=latest,name=iPad (10th generation)'
endif

# If IOS_XCODEBUILD_SIM does not contain a simulator destination, add the default.
ifeq (, $(findstring destination, $(IOS_XCODEBUILD_SIM)))
	IOS_XCODEBUILD_SIM += \
	-destination 'platform=iOS Simulator,OS=latest,name=iPhone 14 Pro'
else
	IOS_XCODEBUILD_SIM += -parallel-testing-enabled YES
endif

ifneq ($(ONLY_TESTING),)
	IOS_XCODEBUILD_SIM += -only-testing:$(ONLY_TESTING)
endif

ifneq ($(SKIP_TESTING),)
	IOS_XCODEBUILD_SIM += -skip-testing:$(SKIP_TESTING)
endif

ifneq ($(CI),)
	IOS_XCODEBUILD_SIM += -xcconfig platform/darwin/ci.xcconfig
endif

$(IOS_PROJ_PATH): $(IOS_USER_DATA_PATH)/WorkspaceSettings.xcsettings $(BUILD_DEPS)
	mkdir -p $(IOS_OUTPUT_PATH)
	(cd $(IOS_OUTPUT_PATH) && $(CMAKE) -G Xcode ../../../../ \
		-DCMAKE_SYSTEM_NAME=iOS )

$(IOS_USER_DATA_PATH)/WorkspaceSettings.xcsettings: platform/ios/WorkspaceSettings.xcsettings
	mkdir -p "$(IOS_USER_DATA_PATH)"
	cp platform/ios/WorkspaceSettings.xcsettings "$@"

.PHONY: ios
ios: $(IOS_PROJ_PATH)
	set -o pipefail && $(IOS_XCODEBUILD_SIM) -scheme 'CI' build $(XCPRETTY)

#  make iproj
.PHONY: iproj
iproj: $(IOS_PROJ_PATH) style-code
	@#   Check if Xcode should run in headless mode
	@#     If CI=1, then just perform `make iproj`
	@#       else, use `xed` to open the XCworkspace
	@if [ $(CI) == 1 ]; then echo "make iproj CI=1.  Running in headless mode, skip opening .xcworkspace in Xcode.  To open Xcode use 'xed $(IOS_WORK_PATH)'"; else echo "make iproj. Opening MapLibre in Xcode using:  'xed $(IOS_WORK_PATH)'.  To skip opening Xcode, use 'make iproj CI=1'"; xed $(IOS_WORK_PATH);  fi;
	@echo "## Xcode dependencies"
	@xcodebuild -version
	@uname -a
	@sw_vers
	@arch
	@brew list --versions cmake ccache pkg-config glfw

.PHONY: ios-lint-plist
ios-lint-plist:
	find platform/ios/framework -type f -name '*.plist' | xargs plutil -lint
	find platform/ios/app -type f -name '*.plist' | xargs plutil -lint

.PHONY: ios-lint
ios-lint: ios-pod-lint
	find platform/ios/framework -type f -name '*.plist' | xargs plutil -lint
	find platform/ios/app -type f -name '*.plist' | xargs plutil -lint

.PHONY: ios-pod-lint
ios-pod-lint:
	./platform/ios/scripts/lint-podspecs.js

#  make ios-sizetest
.PHONY: ios-sizetest
ios-sizetest:
	./scripts/size-check.sh ${DEV_TEAM}

#  make ios-uitest
.PHONY: ios-uitest
ios-uitest: $(IOS_PROJ_PATH)
	set -o pipefail && $(IOS_XCODEBUILD_SIM) -scheme 'iosapp' test $(XCPRETTY)
	echo "You can review uitest results:  open $(IOS_OUTPUT_PATH)/Logs/Test"

.PHONY: ios-test
ios-test: style-code $(IOS_PROJ_PATH)
	set -o pipefail && $(IOS_XCODEBUILD_SIM) -scheme 'CI' test $(XCPRETTY)

.PHONY: ios-integration-test
ios-integration-test: $(IOS_PROJ_PATH)
	set -o pipefail && $(IOS_XCODEBUILD_SIM) -scheme 'Integration Test Harness' test $(XCPRETTY)

.PHONY: ios-sanitize
ios-sanitize: $(IOS_PROJ_PATH)
	set -o pipefail && $(IOS_XCODEBUILD_SIM) -scheme 'CI' -enableThreadSanitizer YES -enableUndefinedBehaviorSanitizer YES test $(XCPRETTY)

.PHONY: ios-sanitize-address
ios-sanitize-address: $(IOS_PROJ_PATH)
	set -o pipefail && $(IOS_XCODEBUILD_SIM) -scheme 'CI' -enableAddressSanitizer YES test $(XCPRETTY)

.PHONY: ios-static-analyzer
ios-static-analyzer: $(IOS_PROJ_PATH)
	set -o pipefail && $(IOS_XCODEBUILD_SIM) analyze -scheme 'CI' test $(XCPRETTY)

.PHONY: ios-install-simulators
ios-install-simulators:
	xcversion simulators --install="iOS 11.4" || true
	xcversion simulators --install="iOS 10.3.1" || true
	xcversion simulators --install="iOS 9.3" || true

.PHONY: ipackage
ipackage: ipackage*
ipackage%:
	@echo make ipackage is deprecated — use make iframework.

.PHONY: iframework
iframework: $(IOS_PROJ_PATH) style-code
	FORMAT=$(FORMAT) BUILD_DEVICE=$(BUILD_DEVICE) SYMBOLS=$(SYMBOLS) BUILD_DOCS=$(BUILD_DOCS) \
	./platform/ios/scripts/package.sh

#  make xcframework
.PHONY: xcframework
xcframework: $(IOS_PROJ_PATH) style-code
	FORMAT=$(FORMAT) SYMBOLS=$(SYMBOLS) BUILD_DOCS=$(BUILD_DOCS) \
	./platform/ios/scripts/xcpackage.sh

.PHONY: ideploy
ideploy:
	caffeinate -i ./platform/ios/scripts/deploy-packages.sh

#  make idocument
#   open documentation/ 
.PHONY: idocument
idocument: style-code
	OUTPUT=$(OUTPUT) ./platform/ios/scripts/document.sh

.PHONY: darwin-check-public-symbols
darwin-check-public-symbols:
	node platform/darwin/scripts/check-public-symbols.js macOS iOS

endif

ios-docs: $(IOS_PROJ_PATH) style-code
	npm ci --prefix ../.. --ignore-scripts
	$(IOS_XCODEBUILD_SIM) docbuild -scheme CI

ios-docs-static: ios-docs
	$(xcrun --find docc) process-archive transform-for-static-hosting \
		$(IOS_OUTPUT_PATH)/Debug-iphonesimulator/Mapbox.doccarchive \
		--output-path $(IOS_OUTPUT_PATH)/static-docs

#### macOS targets ############################################################

ifeq ($(HOST_PLATFORM), macos)

MACOS_OUTPUT_PATH = build/macos
MACOS_PROJ_PATH = $(MACOS_OUTPUT_PATH)/Mapbox\ GL\ Native.xcodeproj
MACOS_WORK_PATH = platform/macos/macos.xcworkspace
MACOS_USER_DATA_PATH = $(MACOS_WORK_PATH)/xcuserdata/$(USER).xcuserdatad

MACOS_XCODEBUILD = xcodebuild \
	-derivedDataPath $(MACOS_OUTPUT_PATH) \
	-configuration $(BUILDTYPE) \
	-workspace $(MACOS_WORK_PATH) \
	-jobs $(JOBS)

ifneq ($(CI),)
	MACOS_XCODEBUILD += -xcconfig platform/darwin/ci.xcconfig
endif

$(MACOS_PROJ_PATH): $(MACOS_USER_DATA_PATH)/WorkspaceSettings.xcsettings $(BUILD_DEPS)
	mkdir -p $(MACOS_OUTPUT_PATH)
	(cd $(MACOS_OUTPUT_PATH) && $(CMAKE) -G Xcode ../../../../ \
		-DCMAKE_SYSTEM_NAME=Darwin )

$(MACOS_USER_DATA_PATH)/WorkspaceSettings.xcsettings: platform/macos/WorkspaceSettings.xcsettings
	mkdir -p "$(MACOS_USER_DATA_PATH)"
	cp platform/macos/WorkspaceSettings.xcsettings "$@"

.PHONY: macos
macos: $(MACOS_PROJ_PATH)
	set -o pipefail && $(MACOS_XCODEBUILD) -scheme 'CI' build $(XCPRETTY)

.PHONY: xproj
xproj: $(MACOS_PROJ_PATH) style-code
	xed $(MACOS_WORK_PATH)

#  make macos-test
.PHONY: macos-test
macos-test: style-code $(MACOS_PROJ_PATH)
	set -o pipefail && $(MACOS_XCODEBUILD) -scheme 'CI' test $(XCPRETTY)

.PHONY: macos-lint
macos-lint:
	find platform/macos -type f -name '*.plist' | xargs plutil -lint

.PHONY: xpackage
xpackage: $(MACOS_PROJ_PATH)
	SYMBOLS=$(SYMBOLS) ./platform/macos/scripts/package.sh

.PHONY: xdeploy
xdeploy:
	caffeinate -i ./platform/macos/scripts/deploy-packages.sh

.PHONY: xdocument
xdocument:
	OUTPUT=$(OUTPUT) ./platform/macos/scripts/document.sh

.PHONY: genstrings
genstrings:
	genstrings -u -o platform/macos/sdk/Base.lproj platform/darwin/src/*.{m,mm}
	genstrings -u -o platform/macos/sdk/Base.lproj platform/macos/src/*.{m,mm}
	genstrings -u -o platform/ios/resources/Base.lproj platform/ios/src/*.{m,mm}
	-find platform/ios/resources platform/macos/sdk -path '*/Base.lproj/*.strings' -exec \
		textutil -convert txt -extension strings -inputencoding UTF-16 -encoding UTF-8 {} \;
	mv platform/macos/sdk/Base.lproj/Foundation.strings platform/darwin/resources/Base.lproj/

endif

#### Miscellaneous targets #####################################################

.PHONY: node-modules
node-modules:
	npm ci --prefix ../.. --ignore-scripts

.PHONY: style-code
style-code: node-modules
	node ../../scripts/generate-style-code.js
	node platform/darwin/scripts/generate-style-code.js
	node platform/darwin/scripts/update-examples.js

.PHONY: codestyle
codestyle:
	scripts/codestyle.sh

# make clean
.PHONY: clean
clean:
	-rm -rf ./build \
	        ./lib/*.node 

.PHONY: distclean
distclean: clean
	-rm -rf ./mason_packages
	-rm -rf ./node_modules
