# Packaging: DRIVE.app and DRIVE.dmg on macOS, DRIVE-windows-x64.zip on
# Windows.  The game itself is built by build.macos.sh / build.linux.sh /
# build.windows.sh.
-include .env

APP_NAME = DRIVE
VERSION  = 1.0
BUILD    = build
APP      = $(BUILD)/$(APP_NAME).app
DMG      = $(BUILD)/$(APP_NAME).dmg
ZIP      = $(BUILD)/$(APP_NAME)-windows-x64.zip
STAGING  = $(BUILD)/dmg_staging
ZIPDIR   = $(BUILD)/zip_staging
RES      = $(APP)/Contents/Resources

.PHONY: all binaries icon app dmg release zip clean

all: dmg

binaries:
	./build.macos.sh

icon: $(BUILD)/$(APP_NAME).icns

$(BUILD)/$(APP_NAME).icns: drive.png
	@mkdir -p $(BUILD)/icon.iconset
	sips -c 795 795 drive.png --out $(BUILD)/icon.iconset/base.png >/dev/null
	@for s in 16 32 128 256 512; do \
	    sips -z $$s $$s $(BUILD)/icon.iconset/base.png \
	        --out $(BUILD)/icon.iconset/icon_$${s}x$${s}.png >/dev/null; \
	    sips -z $$((s*2)) $$((s*2)) $(BUILD)/icon.iconset/base.png \
	        --out $(BUILD)/icon.iconset/icon_$${s}x$${s}@2x.png >/dev/null; \
	done
	@rm $(BUILD)/icon.iconset/base.png
	iconutil -c icns $(BUILD)/icon.iconset -o $@
	@rm -rf $(BUILD)/icon.iconset

# The client is the bundle executable: notarization wants a real Mach-O
# there, not a launcher script.  It finds Contents/Resources on its own and
# starts the hidden server from there.
define build_app
	rm -rf $(APP)
	mkdir -p $(APP)/Contents/MacOS $(RES)
	cp DRIVE/drive $(APP)/Contents/MacOS/$(APP_NAME)
	cp DRIVE/drive_server DRIVE/drive_help DRIVE/drive_blocks \
	   DRIVE/sound.cnf $(RES)/
	cp -R DRIVE/pixmaps DRIVE/scenes DRIVE/constructs DRIVE/textures $(RES)/
	cp $(BUILD)/$(APP_NAME).icns $(RES)/
	printf '%s\n' \
	  '<?xml version="1.0" encoding="UTF-8"?>' \
	  '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' \
	  '<plist version="1.0"><dict>' \
	  '  <key>CFBundleExecutable</key><string>$(APP_NAME)</string>' \
	  '  <key>CFBundleIdentifier</key><string>com.github.tenox7.drive</string>' \
	  '  <key>CFBundleName</key><string>$(APP_NAME)</string>' \
	  '  <key>CFBundleDisplayName</key><string>$(APP_NAME)</string>' \
	  '  <key>CFBundleVersion</key><string>$(VERSION)</string>' \
	  '  <key>CFBundleShortVersionString</key><string>$(VERSION)</string>' \
	  '  <key>CFBundlePackageType</key><string>APPL</string>' \
	  '  <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>' \
	  '  <key>CFBundleIconFile</key><string>$(APP_NAME)</string>' \
	  '  <key>LSMinimumSystemVersion</key><string>11.0</string>' \
	  '  <key>NSHighResolutionCapable</key><true/>' \
	  '  <key>NSPrincipalClass</key><string>NSApplication</string>' \
	  '</dict></plist>' > $(APP)/Contents/Info.plist
	printf 'APPL????' > $(APP)/Contents/PkgInfo
endef

# Sign inside out: the nested server first, then the bundle.
define sign_app
	codesign --force $(1) --sign $(2) $(RES)/drive_server
	codesign --force $(1) --sign $(2) $(APP)
	codesign --verify --strict --verbose=2 $(APP)
endef

define build_dmg
	rm -rf $(STAGING) $(1)
	mkdir -p $(STAGING)
	cp -R $(APP) $(STAGING)/
	ln -s /Applications $(STAGING)/Applications
	hdiutil create -volname "$(APP_NAME)" -srcfolder $(STAGING) -ov \
	    -format UDZO $(1)
	rm -rf $(STAGING)
endef

app: binaries icon
	$(call build_app)
	$(call sign_app,,-)
	@otool -L $(APP)/Contents/MacOS/$(APP_NAME) | grep -v /System | grep -v /usr/lib || true
	@du -sh $(APP)

dmg: app
	$(call build_dmg,$(DMG))
	@ls -lh $(DMG)

release: binaries icon
	@test -n "$(DEV_ID)" || { echo "DEV_ID not set - copy .env.example to .env and fill in"; exit 1; }
	@test -n "$(NOTARY_PROFILE)" || { echo "NOTARY_PROFILE not set - copy .env.example to .env and fill in"; exit 1; }
	$(call build_app)
	$(call sign_app,--options runtime --timestamp,"$(DEV_ID)")
	$(call build_dmg,$(DMG))
	codesign --force --timestamp --sign "$(DEV_ID)" $(DMG)
	xcrun notarytool submit $(DMG) --keychain-profile "$(NOTARY_PROFILE)" --wait
	xcrun stapler staple $(DMG)
	spctl -a -t open --context context:primary-signature -v $(DMG)
	@echo "Signed + notarized: $(DMG)"

# Windows: zip up what build.windows.sh left in DRIVE, laid out the way the
# .bat files expect.  Run this under MSYS2, after build.windows.sh.
zip:
	@test -f DRIVE/drive.exe || { echo "run ./build.windows.sh first"; exit 1; }
	rm -rf $(ZIPDIR) $(ZIP)
	mkdir -p $(ZIPDIR)/drive/DRIVE
	cp run.bat justdrive.bat $(ZIPDIR)/drive/
	cp DRIVE/drive.exe DRIVE/drive_server.exe DRIVE/drive_help \
	   DRIVE/drive_blocks DRIVE/sound.cnf $(ZIPDIR)/drive/DRIVE/
	cp -R DRIVE/pixmaps DRIVE/scenes DRIVE/constructs DRIVE/textures \
	   $(ZIPDIR)/drive/DRIVE/
	find $(ZIPDIR) -name .DS_Store -delete
	(cd $(ZIPDIR); zip -qr ../$(notdir $(ZIP)) drive)
	rm -rf $(ZIPDIR)
	@ls -lh $(ZIP)

clean:
	rm -rf $(BUILD)
