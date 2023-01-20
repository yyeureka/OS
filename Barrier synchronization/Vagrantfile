# -*- mode: ruby -*-
# vi: set ft=ruby :

# Property of the CS 6210 teaching staff. Please do not redistribute.

$install = <<-SCRIPT
  apt-get -y update && \
  DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" upgrade && \
  DEBIAN_FRONTEND=noninteractive apt-get -y install \
  build-essential \
  mpich \
  tmux \
  git \
  zip \
  unzip && ln -s -f /usr/bin/python3 /usr/bin/python
SCRIPT

Vagrant.configure(2) do |config|
  config.vm.box = "bento/ubuntu-18.04"

  config.vm.provider "virtualbox" do |vb|
    vb.gui = false
    vb.memory = "1024"
    vb.cpus = "2"
  end

  config.vm.provision "shell", inline: $install
end
