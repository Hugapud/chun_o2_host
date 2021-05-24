#ifndef __CHUN_O2_NODE_HPP_
#define __CHUN_O2_NODE_HPP_
#include <vector>
#include <stdint-gcc.h>
#include <algorithm>
#include <logger.hpp>
#include <at24c04.h>

#define ADDR_Ax 0b000 //A2, A1, A0
#define ADDR 0x50

class Node
{
public:
    Node() : id_{0, 0, 0}
    {
    }
    Node(uint32_t *id) : id_{id[0], id[1], id[2]}
    {
    }
    Node(const Node &node) : id_{node.getId()[0], node.getId()[1], node.getId()[2]} {

                             };
    Node(Node &&) = delete;

    bool compareId(const uint32_t *id) const
    {
        return (id_[0] == id[0] && id_[1] == id[1] && id_[2] == id[2]);
    }

    bool operator==(const Node &node) const
    {
        return compareId(node.getId());
    }

    void setO2Num(float o2Num)
    {
        Node::o2Num_ = o2Num;
    }

    float getO2Num()
    {
        return o2Num_;
    }

    const uint32_t *getId() const
    {
        return id_;
    }

private:
    const uint32_t id_[3];
    float o2Num_;
};

class NodeList
{
public:
    NodeList() : at24c04_(ADDR)
    {
        at24c04_.initialize();
        // 读取EEPROM中的阈值
        union transfer
        {
            float f;
            uint8_t data[4];
        } t;
        at24c04_.readBytes(0, 4, t.data);
        threshold_ = t.f;
        if (threshold_ >= 100)
        {
            threshold_ = 100.0;
        }
        if (threshold_ <= 0)
        {
            threshold_ = 0.0;
        }
    }
    Node &getNode(uint32_t *id)
    {
        auto check = [id](Node &node) {
            return node.compareId(id);
        };
        std::vector<Node>::iterator it = std::find_if(list_.begin(), list_.end(), check);
        if (it != list_.end())
        {
            return *it;
        }
        else
        {
            Node node(id);
            list_.push_back(node);
            return *(list_.end() - 1);
        }
    }

    Node &getNode(uint8_t *id)
    {
        uint32_t id_[ID_LENGTH];
        getID(id, id_);
        return getNode(id_);
    }

    void getNodeState(std::vector<float> &o2)
    {
        o2.clear();
        for (auto &node : list_)
        {
            o2.push_back(node.getO2Num());
        }
    }

    bool isAlarm()
    {
        for (auto &node : list_)
        {
            if (node.getO2Num() < threshold_)
            {
                LOGGER << "ALARM!";
                return true;
            }
        }
        return false;
    }

    size_t size() const
    {
        return list_.size();
    }

    float getThreshold()
    {
        return threshold_;
    }

    void setThreshold(float threshold)
    {
        threshold_ = threshold;
        // 将阈值写入EEPROM
        union transfer
        {
            float f;
            uint8_t data[4];
        } t;
        t.f = threshold_;
        at24c04_.writeBytes(0, 4, t.data);
    }

private:
    std::vector<Node> list_;
    const size_t ID_LENGTH = 3;
    float threshold_;
    AT24C04 at24c04_;

    uint8_t getID(uint8_t *pos, uint32_t *id)
    {
        uint8_t sum = 0;
        union transfer
        {
            uint32_t id;
            uint8_t data[4];
        };
        for (size_t i = 0; i != ID_LENGTH; i++)
        {
            transfer t;
            t.data[0] = *(pos + i * 4 + 0);
            t.data[1] = *(pos + i * 4 + 1);
            t.data[2] = *(pos + i * 4 + 2);
            t.data[3] = *(pos + i * 4 + 3);
            sum += t.data[0] + t.data[1] + t.data[2] + t.data[3];
            *(id + i) = t.id;
        }
        return sum;
    }

    
};

#endif