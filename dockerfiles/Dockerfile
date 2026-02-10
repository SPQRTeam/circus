FROM ros:humble-ros-base
ENV DEBIAN_FRONTEND=noninteractive

# install dependencies
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
    ament-cmake \
    && rm -rf /var/lib/apt/lists/*

# copy configs
RUN mkdir /opt/booster
RUN mkdir /opt/booster/configs
COPY configs/system_settings_config.yaml /opt/booster/configs
COPY booster.conf /etc/supervisor/conf.d/

# copy and install booster sdk
COPY booster_robotics_sdk /opt/booster/sdk
RUN /opt/booster/sdk/install.sh

# compile examples too (loco client in particular)
RUN mkdir opt/booster/sdk/build
RUN cd opt/booster/sdk/build && cmake .. && make

# copy, compile and install booster ros sdk
COPY booster_robotics_sdk_ros2 /opt/booster/sdk_ros2

# Build with minimal optimization and sequential execution
RUN /bin/bash -c "source /opt/ros/humble/setup.bash; cd /opt/booster/sdk_ros2 && colcon build"

COPY LocoApiPackage /opt/booster/LocoApiPackage
# RUN /bin/bash -c "source /opt/ros/humble/setup.bash; cd /opt/booster/LocoApiPackage && colcon build --packages-select booster_msgs

# add custom bashrc
COPY configs/bashrc /root/.bashrc_addendum
RUN echo ". ~/.bashrc_addendum" >> ~/.bashrc

RUN mkdir /app
COPY entrypoint.sh /app
RUN chmod +x /app/entrypoint.sh

COPY delayed_start.sh /app
RUN chmod +x /app/delayed_start.sh

# used to test communication, can be removed at a later date if so desired
RUN apt install -y iputils-ping netcat net-tools


CMD ["/app/entrypoint.sh"]
