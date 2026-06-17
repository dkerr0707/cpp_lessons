// Connect to the first camera and dump every GenICam node from all three
// node maps (TL Device, TL Stream, Device) — name, value, type-specific
// constraints, and the human-readable description.
//
// Output is verbose; pipe to a pager or a file:
//   ./bin/camera_info | less
//   ./bin/camera_info > info.txt

#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>

#include <iostream>

static void print_indent(int depth) {
    for (int i = 0; i < depth; ++i) std::cout << "  ";
}

static void walk_node(Spinnaker::GenApi::INode* node, int depth);

static void walk_category(Spinnaker::GenApi::CCategoryPtr cat, int depth) {
    Spinnaker::GenApi::FeatureList_t features;
    cat->GetFeatures(features);
    for (Spinnaker::GenApi::INode* child : features) {
        walk_node(child, depth);
    }
}

static void walk_node(Spinnaker::GenApi::INode* node, int depth) {
    if (!Spinnaker::GenApi::IsImplemented(node)) return;

    print_indent(depth);
    std::cout << node->GetName().c_str();

    const Spinnaker::GenApi::EInterfaceType type = node->GetPrincipalInterfaceType();
    switch (type) {
        case Spinnaker::GenApi::intfICategory: {
            std::cout << "  [category]\n";
            walk_category(static_cast<Spinnaker::GenApi::CCategoryPtr>(node), depth + 1);
            return;  // skip description on categories (often empty / noisy)
        }
        case Spinnaker::GenApi::intfIInteger: {
            Spinnaker::GenApi::CIntegerPtr p = node;
            std::cout << " = ";
            if (Spinnaker::GenApi::IsReadable(p)) std::cout << p->GetValue();
            else std::cout << "(unreadable)";
            std::cout << "  [int " << p->GetMin() << ".." << p->GetMax();
            if (p->GetInc() > 1) std::cout << " step " << p->GetInc();
            const Spinnaker::GenICam::gcstring unit = p->GetUnit();
            if (unit.size()) std::cout << " " << unit.c_str();
            std::cout << "]\n";
            break;
        }
        case Spinnaker::GenApi::intfIFloat: {
            Spinnaker::GenApi::CFloatPtr p = node;
            std::cout << " = ";
            if (Spinnaker::GenApi::IsReadable(p)) std::cout << p->GetValue();
            else std::cout << "(unreadable)";
            std::cout << "  [float " << p->GetMin() << ".." << p->GetMax();
            const Spinnaker::GenICam::gcstring unit = p->GetUnit();
            if (unit.size()) std::cout << " " << unit.c_str();
            std::cout << "]\n";
            break;
        }
        case Spinnaker::GenApi::intfIString: {
            Spinnaker::GenApi::CStringPtr p = node;
            std::cout << " = ";
            if (Spinnaker::GenApi::IsReadable(p)) {
                std::cout << '"' << p->GetValue().c_str() << '"';
            } else {
                std::cout << "(unreadable)";
            }
            std::cout << "\n";
            break;
        }
        case Spinnaker::GenApi::intfIBoolean: {
            Spinnaker::GenApi::CBooleanPtr p = node;
            std::cout << " = ";
            if (Spinnaker::GenApi::IsReadable(p)) {
                std::cout << (p->GetValue() ? "true" : "false");
            } else {
                std::cout << "(unreadable)";
            }
            std::cout << "\n";
            break;
        }
        case Spinnaker::GenApi::intfIEnumeration: {
            Spinnaker::GenApi::CEnumerationPtr p = node;
            std::cout << " = ";
            if (Spinnaker::GenApi::IsReadable(p)) {
                Spinnaker::GenApi::CEnumEntryPtr cur = p->GetCurrentEntry();
                if (cur.IsValid()) std::cout << cur->GetSymbolic().c_str();
                else std::cout << "(none)";
            } else {
                std::cout << "(unreadable)";
            }
            std::cout << "  [enum:";
            Spinnaker::GenApi::NodeList_t entries;
            p->GetEntries(entries);
            bool first = true;
            for (Spinnaker::GenApi::INode* entry : entries) {
                Spinnaker::GenApi::CEnumEntryPtr ee = entry;
                if (!Spinnaker::GenApi::IsAvailable(ee)) continue;
                std::cout << (first ? " " : ", ") << ee->GetSymbolic().c_str();
                first = false;
            }
            std::cout << "]\n";
            break;
        }
        case Spinnaker::GenApi::intfICommand:
            std::cout << "  [command]\n";
            break;
        case Spinnaker::GenApi::intfIRegister:
            std::cout << "  [register]\n";
            break;
        default:
            std::cout << "  [type=" << static_cast<int>(type) << "]\n";
            break;
    }

    const Spinnaker::GenICam::gcstring desc = node->GetDescription();
    if (desc.size()) {
        print_indent(depth + 1);
        std::cout << "# " << desc.c_str() << '\n';
    }
    std::cout << '\n';
}

static void dump_nodemap(const char* label, Spinnaker::GenApi::INodeMap& nm) {
    std::cout << "\n========== " << label << " ==========\n";
    Spinnaker::GenApi::CCategoryPtr root = nm.GetNode("Root");
    if (root.IsValid()) walk_category(root, 0);
    else std::cout << "(no Root category)\n";
}

int main() {
    Spinnaker::SystemPtr system = Spinnaker::System::GetInstance();

    Spinnaker::CameraList cameras = system->GetCameras();
    if (cameras.GetSize() == 0) {
        cameras.Clear();
        system->ReleaseInstance();
        std::cerr << "No cameras found.\n";
        return 1;
    }

    try {
        Spinnaker::CameraPtr cam = cameras.GetByIndex(0);

        // TL Device map is available before Init().
        dump_nodemap("Transport Layer Device", cam->GetTLDeviceNodeMap());

        cam->Init();
        dump_nodemap("Transport Layer Stream", cam->GetTLStreamNodeMap());
        dump_nodemap("Device", cam->GetNodeMap());
        cam->DeInit();
        cam = nullptr;
    }
    catch (Spinnaker::Exception& e) {
        std::cerr << "Spinnaker error: " << e.what() << '\n';
        cameras.Clear();
        system->ReleaseInstance();
        return 2;
    }

    cameras.Clear();
    system->ReleaseInstance();
    return 0;
}
