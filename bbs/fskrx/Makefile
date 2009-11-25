# FSK Rx Test Routine
# Copyright (C) 2009 Anneal Technology Inc. All rights reserved.

include $(TOPDIR)/rules.mk

PKG_NAME:=fskrx
PKG_RELEASE:=1

PKG_SOURCE_URL:=git://github.com/yclee/fskrx.git
PKG_REV:=HEAD
PKG_SOURCE:=$(PKG_NAME)-$(PKG_REV).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE_VERSION:=$(PKG_REV)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  CATEGORY:=Test
  URL:=http://www.anneal.com.tw/
  TITLE:=FSK Rx Test Routine
  DEPENDS:=+spandsp
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
