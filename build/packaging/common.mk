# build/packaging/common.mk
# Các biến dùng chung cho các package TizenOS
TIZENOS_VERSION = 1.0.0
CODENAME = alpha
MAINTAINER = "TizenOS Team <dev@tizenos.org>"

# Tự động nhận diện kiến trúc
ARCH ?= $(shell dpkg-architecture -qDEB_BUILD_ARCH)

# Mục tiêu mặc định
.PHONY: all deb udeb clean lint

all: deb

# Build package .deb (dpkg-buildpackage)
deb:
	@echo "Đang build package .deb cho kiến trúc $(ARCH)..."
	dpkg-buildpackage -us -uc -b -a$(ARCH)

# Build udeb (dành cho Debian Installer)
udeb:
	@echo "Đang build package .udeb cho kiến trúc $(ARCH)..."
	dpkg-buildpackage -us -uc -b -a$(ARCH) -d

# Dọn dẹp thư mục sau khi build
clean:
	@echo "Dọn dẹp thư mục..."
	dh_clean
	rm -f ../*.deb ../*.udeb ../*.buildinfo ../*.changes

# Chạy lintian để kiểm tra chất lượng package
lint: deb
	@echo "Kiểm tra package bằng lintian..."
	lintian ../*.deb
