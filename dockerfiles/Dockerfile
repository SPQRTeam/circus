FROM ros:humble-ros-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    sudo \
    curl \
    supervisor \
    gnupg2 \
    lsb-release \
    build-essential \
    libtinyxml2-dev \
    libtinyxml2-9 \
    libpulse0 \
    libpulse-mainloop-glib0 \
    libasound2 \
    && rm -rf /var/lib/apt/lists/*

# Necessary to avoid permission issues with booster-motion
RUN usermod -aG sudo root
RUN echo "root ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN mkdir /opt/booster
RUN mkdir /opt/booster/configs
COPY configs/system_settings_config.yaml /opt/booster/configs
COPY booster.conf /etc/supervisor/conf.d/

RUN echo "source /opt/ros/humble/setup.sh" >> ~/.bashrc

CMD ["/usr/bin/supervisord", "-n"]