#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>

#include "sensors/Sensor.h"

using namespace Eigen;

namespace spqr {

// Ground-truth contact force on the two feet

class ContactForce : public Sensor {
    public:
        ContactForce(mjModel* mujModel, mjData* mujData, const char* leftFootBody, const char* rightFootBody)
            : mujModel(mujModel), mujData(mujData) {
            leftBodyId = mj_name2id(mujModel, mjOBJ_BODY, leftFootBody);
            rightBodyId = mj_name2id(mujModel, mjOBJ_BODY, rightFootBody);
        }

        void doUpdate() override {
            leftForce.setZero();
            rightForce.setZero();

            for (int i = 0; i < mujData->ncon; ++i) {
                const mjContact& c = mujData->contact[i];
                if (c.efc_address < 0)
                    continue;  // contact not included in the constraint solver

                const int body1 = mujModel->geom_bodyid[c.geom1];
                const int body2 = mujModel->geom_bodyid[c.geom2];

                Vector3d* target = nullptr;
                if (body1 == leftBodyId || body2 == leftBodyId)
                    target = &leftForce;
                else if (body1 == rightBodyId || body2 == rightBodyId)
                    target = &rightForce;
                else
                    continue;  // contact does not involve a foot

                mjtNum wrench[6], rotationContact2World[9], forceWorldRF[3];
                mj_contactForce(mujModel, mujData, i, wrench);
                mju_transpose(rotationContact2World, c.frame, 3, 3); // get rotation contact RF wrt world RF
                mju_mulMatVec(forceWorldRF, rotationContact2World, wrench, 3, 3);

                *target += Vector3d(forceWorldRF[0], forceWorldRF[1], forceWorldRF[2]);
            }
        }

        msgpack::object doSerialize(msgpack::zone& z) override {
            std::vector<double> left_vec = {leftForce(0), leftForce(1), leftForce(2)};
            std::vector<double> right_vec = {rightForce(0), rightForce(1), rightForce(2)};

            std::map<std::string, msgpack::object> contact_data;
            contact_data["left"] = msgpack::object(left_vec, z);
            contact_data["right"] = msgpack::object(right_vec, z);
            return msgpack::object(contact_data, z);
        }

        Vector3d getLeftForce() const {
            return leftForce;
        }
        Vector3d getRightForce() const {
            return rightForce;
        }

    private:
        Vector3d leftForce = Vector3d::Zero();   // [fx, fy, fz] world-frame GRF on the left foot
        Vector3d rightForce = Vector3d::Zero();  // [fx, fy, fz] world-frame GRF on the right foot

        mjModel* mujModel;
        mjData* mujData;

        int leftBodyId = -1;
        int rightBodyId = -1;
};
}  // namespace spqr
