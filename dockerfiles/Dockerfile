FROM ros:humble-ros-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
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
    libxcursor1 \
    libxinerama1 \
    libxi6 \
    libxfixes3 \
    libxrandr2 \
    libxss1 \
    libxxf86vm1 \
    libdrm2 \
    libgbm1 \
    libwayland-egl1 \
    libwayland-client0 \
    libwayland-cursor0 \
    libxkbcommon0 \
    libdecor-0-0 \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir /opt/booster
RUN mkdir /opt/booster/configs
COPY configs/system_settings_config.yaml /opt/booster/configs
COPY booster.conf /etc/supervisor/conf.d/
COPY configs/bashrc /root/.bashrc_addendum

RUN echo ". ~/.bashrc_addendum" >> ~/.bashrc

CMD ["/usr/bin/supervisord", "-n", "-c", "/etc/supervisor/conf.d/booster.conf"]
