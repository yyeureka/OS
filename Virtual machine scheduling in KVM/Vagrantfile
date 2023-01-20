# -*- mode: ruby -*-
# vi: set ft=ruby :

# Property of the CS 6210 teaching staff. Please do not redistribute.

$install = <<-SCRIPT
  apt-get -y update && \
  DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" upgrade && \
  DEBIAN_FRONTEND=noninteractive apt-get -y install \
  build-essential \
  qemu-kvm \
  uvtool \
  libvirt-dev \
  libvirt-daemon-system \
  python3-libvirt \
  python-is-python3 \
  python3-pandas \
  python3-matplotlib \
  virt-top \
  virt-manager \
  tmux \
  git \
  zip \
  unzip && \
  uvt-simplestreams-libvirt sync release=bionic arch=amd64
SCRIPT

$post_install = <<-SCRIPT
  echo "Host 192.168.*" > $HOME/.ssh/config && \
  echo -e "\tStrictHostKeyChecking no" >> $HOME/.ssh/config && \
  echo -e "\tUserKnownHostsFile /dev/null" >> $HOME/.ssh/config && \
  echo -e "\tLogLevel ERROR" >> $HOME/.ssh/config && \
  ssh-keygen -t rsa -q -f "$HOME/.ssh/id_rsa" -N ""
SCRIPT

Vagrant.configure(2) do |config|
  config.vm.box = "bento/ubuntu-20.04"

  config.vm.provider "virtualbox" do |vb|
    vb.gui = false
    vb.memory = "6144"
    vb.cpus = "4"
    vb.customize ["modifyvm", :id, "--nested-hw-virt", "on"]
  end

  config.vm.provision "shell", inline: $install
  config.vm.provision "shell", privileged: false, inline: $post_install
end
