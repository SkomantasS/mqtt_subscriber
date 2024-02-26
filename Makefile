include $(TOPDIR)/rules.mk

PKG_NAME:=mqtt_sub
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/mqtt_sub
	CATEGORY:=Base system
	TITLE:=mqtt_sub
	DEPENDS:=+libmosquitto +libubus +libubox +libuci +libcurl +libblobmsg-json
endef

define Package/mqtt_sub/description
	This is a simple mqtt subscriber
endef

define Package/mqtt_sub/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/ssl/certs
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mqtt_sub $(1)/usr/bin
	$(INSTALL_BIN) ./files/ca.cert.pem $(1)/etc/ssl/certs/ca.cert.pem
	$(INSTALL_BIN) ./files/client.cert.pem $(1)/etc/ssl/certs/client.cert.pem
	$(INSTALL_BIN) ./files/client.key.pem $(1)/etc/ssl/certs/client.key.pem
	$(INSTALL_CONF) ./files/mqtt_sub.config $(1)/etc/config/mqtt_sub
	$(INSTALL_BIN) ./files/mqtt_sub.init $(1)/etc/init.d/mqtt_sub
endef

$(eval $(call BuildPackage,mqtt_sub))