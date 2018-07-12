BASE_DIR        = $(shell pwd)

include $(BASE_DIR)/../../Makefile.custom

VERSION_SHORT   = $(MLSL_MAJOR_VERSION).$(MLSL_MINOR_VERSION)
INSTALL_DIR     = /opt/intel
INSTALL_SUBDIR  = mlsl_$(VERSION_SHORT).$(MLSL_PACKAGE_ID)
INSTALL_FULLDIR = $(INSTALL_DIR)/$(INSTALL_SUBDIR)
ARCH            = x86_64
TEMPORARY_DIR   = $(BASE_DIR)/tmp
TEMPLATES_DIR   = $(BASE_DIR)/templates
STAGING_DIR     = $(BASE_DIR)/../../_staging
RPM_RESULT_DIR  = $(BASE_DIR)/../../_rpm
DEB_RESULT_DIR  = $(BASE_DIR)/../../_deb
REPOS_DIR       = $(BASE_DIR)/../../_repositories
AWS_REPO_DIR    = mlsl/$(MLSL_MAJOR_VERSION)

all: rpm deb yum_repo apt_repo
packages: rpm deb

apt_repo:
	mkdir -p $(TEMPORARY_DIR)/apt/binary
	cp $(DEB_RESULT_DIR)/*.deb $(TEMPORARY_DIR)/apt/binary
	sed -i "s|MLSL_SUBSTITUTE_OFFICIAL_VERSION|$(MLSL_OFFICIAL_VERSION)|g" "$(TEMPORARY_DIR)/release_file.sh"
	cd $(TEMPORARY_DIR)/apt && dpkg-scanpackages -m binary > binary/Packages
	cd $(TEMPORARY_DIR)/apt/binary && $(TEMPORARY_DIR)/release_file.sh
	$(SIGNFILE_PATH) -s cl -cf $(TEMPORARY_DIR)/apt/binary/Release.pgp $(TEMPORARY_DIR)/apt/binary/Release
	$(SIGNFILE_PATH) -s cl -pgp_clear -cf $(TEMPORARY_DIR)/apt/binary/InRelease $(TEMPORARY_DIR)/apt/binary/Release
	mkdir -p $(REPOS_DIR)/apt/$(AWS_REPO_DIR)
	cp -pr $(TEMPORARY_DIR)/apt/* $(REPOS_DIR)/apt/$(AWS_REPO_DIR)

yum_repo:
	mkdir -p $(TEMPORARY_DIR)/yum
	cp $(RPM_RESULT_DIR)/*.rpm $(TEMPORARY_DIR)/yum
	cd $(TEMPORARY_DIR)/yum && createrepo .
	$(SIGNFILE_PATH) -s cl -cf $(TEMPORARY_DIR)/yum/repodata/repomd.xml.asc $(TEMPORARY_DIR)/yum/repodata/repomd.xml
	mkdir -p $(REPOS_DIR)/yum/$(AWS_REPO_DIR)
	cp -pr $(TEMPORARY_DIR)/yum/* $(REPOS_DIR)/yum/$(AWS_REPO_DIR)

rpm: rpm_test
	mkdir -p $(RPM_RESULT_DIR)
	cp $(TEMPORARY_DIR)/_out/build/rpm/RPMS/$(ARCH)/*.rpm $(RPM_RESULT_DIR)
	cp $(SIGNTOOL_DIR)/PUBLIC_KEY.PUB $(RPM_RESULT_DIR)

deb: deb_test
	mkdir -p $(DEB_RESULT_DIR)
	cp $(TEMPORARY_DIR)/_out/build/*.deb $(DEB_RESULT_DIR)
	cp $(SIGNTOOL_DIR)/PUBLIC_KEY.PUB $(DEB_RESULT_DIR)

rpm_test: rpm_sign
	$(SIGNFILE_PATH) -vv -t $(TEMPORARY_DIR)/_out/build/rpm/RPMS/$(ARCH)/*.rpm

deb_test: deb_sign
	$(SIGNFILE_PATH) -vv -t $(TEMPORARY_DIR)/_out/build/*.deb

rpm_sign: rpm_build
	$(SIGNFILE_PATH) -vv $(TEMPORARY_DIR)/_out/build/rpm/RPMS/$(ARCH)/*.rpm

deb_sign: deb_build
	$(SIGNFILE_PATH) -vv $(TEMPORARY_DIR)/_out/build/*.deb

rpm_build: rpm_build_prepare
	rpmbuild -bb -vv --target $(ARCH) \
		--nodeps $(TEMPORARY_DIR)/_out/build/rpm/SPECS/intel-mlsl.spec \
		--define _binary_payload=w9.gzdio \
		--define "_topdir $(TEMPORARY_DIR)/_out/build/rpm" \
		--buildroot $(BASE_DIR)/tmp/_out/build/rpm/SOURCES

deb_build: deb_build_prepare
	cd $(TEMPORARY_DIR)/_out/build; fakeroot dpkg-deb --build debian $(MLSL_DEB_PREFIX_NAME)-$(VERSION_SHORT)-$(MLSL_PACKAGE_ID)_amd64.deb

rpm_build_prepare: templates_prepare
	mkdir -p $(TEMPORARY_DIR)/_out/build/rpm/SOURCES/$(INSTALL_FULLDIR)
	mkdir -p $(TEMPORARY_DIR)/_out/build/rpm/SPECS
	mkdir -p $(TEMPORARY_DIR)/_out/build/rpm/RPMS/$(ARCH)
	chmod -R 755 $(TEMPORARY_DIR)/_out/build/rpm/SOURCES/
	cp -pr $(TEMPORARY_DIR)/_in/* $(TEMPORARY_DIR)/_out/build/rpm/SOURCES/$(INSTALL_FULLDIR)
	cp $(TEMPORARY_DIR)/intel-mlsl.spec $(TEMPORARY_DIR)/_out/build/rpm/SPECS

deb_build_prepare: templates_prepare
	mkdir -p $(TEMPORARY_DIR)/_out/build/debian/$(INSTALL_FULLDIR)
	mkdir -p $(TEMPORARY_DIR)/_out/build/debian/DEBIAN
	cp -pr $(TEMPORARY_DIR)/_in/* $(TEMPORARY_DIR)/_out/build/debian/$(INSTALL_FULLDIR)
	cp $(TEMPORARY_DIR)/control $(TEMPORARY_DIR)/_out/build/debian/DEBIAN
	cp $(TEMPORARY_DIR)/postinst $(TEMPORARY_DIR)/_out/build/debian/DEBIAN
	cp $(TEMPORARY_DIR)/preinst $(TEMPORARY_DIR)/_out/build/debian/DEBIAN
	chmod -R 755 $(TEMPORARY_DIR)/_out/build/debian/DEBIAN
	chmod -R g-s $(TEMPORARY_DIR)/_out/build/debian/DEBIAN

templates_prepare: staging_prepare
	mkdir -p $(TEMPORARY_DIR)/_out
	cp $(TEMPLATES_DIR)/* $(TEMPORARY_DIR)/
	sed -i "s|RPM_NAME|$(MLSL_RPM_PREFIX_NAME)|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|VERSION_SHORT|$(VERSION_SHORT)|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|PACKAGE_ID|$(MLSL_PACKAGE_ID)|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|ARCH|$(ARCH)|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|INSTALL_DIR|$(INSTALL_DIR)|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|INSTALL_SUBDIR|$(INSTALL_SUBDIR)|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|INSTALL_FULLDIR|$(INSTALL_FULLDIR)|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|BUILD_ROOT|$(TEMPORARY_DIR)/_out/build/SOURCES|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|MLSL_SUBSTITUTE_OFFICIAL_VERSION|$(MLSL_OFFICIAL_VERSION)|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|MLSL_SUBSTITUTE_COPYRIGHT_YEAR|$(MLSL_COPYRIGHT_YEAR)|g" "$(TEMPORARY_DIR)/intel-mlsl.spec"
	sed -i "s|DEB_NAME|$(MLSL_RPM_PREFIX_NAME)|g" "$(TEMPORARY_DIR)/control"
	sed -i "s|VERSION_FULL|$(MLSL_FULL_VERSION)|g" "$(TEMPORARY_DIR)/control"
	sed -i "s|ARCH|$(ARCH)|g" "$(TEMPORARY_DIR)/control"
	sed -i "s|MLSL_SUBSTITUTE_OFFICIAL_VERSION|$(MLSL_OFFICIAL_VERSION)|g" "$(TEMPORARY_DIR)/control"
	sed -i "s|INSTALL_DIR|$(INSTALL_DIR)|g" "$(TEMPORARY_DIR)/preinst"
	sed -i "s|INSTALL_DIR|$(INSTALL_DIR)|g" "$(TEMPORARY_DIR)/postinst"
	sed -i "s|INSTALL_SUBDIR|$(INSTALL_SUBDIR)|g" "$(TEMPORARY_DIR)/postinst"

staging_prepare:
	mkdir -p $(TEMPORARY_DIR)/_in
	cp -pr $(STAGING_DIR)/* $(TEMPORARY_DIR)/_in

clean:
	rm -rf $(TEMPORARY_DIR)
	rm -rf $(RPM_RESULT_DIR)
	rm -rf $(DEB_RESULT_DIR)
	rm -rf $(REPOS_DIR)
