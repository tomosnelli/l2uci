include $(TOPDIR)/rules.mk

PKG_NAME:=l2uci
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

# Define build directory
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

define Package/l2uci
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=UCI execution via Layer 2
	DEPENDS:=+libuci
endef

define Package/l2uci/description
	A simple program that uses UCI.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/src
	cp -r ./src/* $(PKG_BUILD_DIR)/src/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/l2uci $(PKG_BUILD_DIR)/src/main.c
endef

define Package/l2uci/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/l2uci $(1)/usr/bin/
endef

$(eval $(call BuildPackage,l2uci))