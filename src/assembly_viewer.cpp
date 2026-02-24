#include <format>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "assembly_viewer.hpp"

namespace {
  constexpr int kMaxMemorySize = 1 << 16;
  std::array<bool, kMaxMemorySize> g_visited;
}

void AssemblyViewer::Initialize(Emulator* emulator) {
  emulator_ = emulator;
}

void AssemblyViewer::Draw() {
  if (ImGui::BeginPopup("Options")) {
    ImGui::Checkbox("Auto-scroll", &auto_scroll_);
    ImGui::EndPopup();
  }

  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("Options");
  }

  if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    ImGuiStyle& style = ImGui::GetStyle();

    // auto line_total_count = static_cast<int>(regs->mem.size() / 2);
    int line_total_count = kMaxMemorySize;
    float line_height = ImGui::GetTextLineHeight();

    ImGuiListClipper clipper;
    clipper.Begin(line_total_count, line_height);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    const auto& regs = emulator_->GetRegisters();

    g_visited.fill(false);

    while (clipper.Step()) {
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i += 1) {
        auto addr = static_cast<u16>(i);
        std::string instruction_text = GetInstruction(addr);
        bool is_current_line = addr == regs.pc;
        bool is_greyed_out = instruction_text.empty() || instruction_text == "NOP";

        if (is_current_line) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0, 0, 1.0));
        }

        ImGui::TextUnformatted(std::format("{:02X}", addr).c_str());

        if (is_current_line) {
          ImGui::PopStyleColor();
        }

        if (is_greyed_out) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
        } else if (is_current_line) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0, 0, 1.0));
        }

        ImGui::SameLine();
        ImGui::TextUnformatted(std::format(":  {:02X}", emulator_->Read8(addr)).c_str());

        ImGui::SameLine();
        ImGui::TextUnformatted("  ");
        ImGui::SameLine();

        ImGui::TextUnformatted(instruction_text.c_str());

        if (is_greyed_out || is_current_line) {
          ImGui::PopStyleColor();
        }

        g_visited[addr] = true;
      }
    }

    if (auto_scroll_) {
      float scroll_y = line_height * static_cast<float>(regs.pc);
      ImGui::SetScrollY(scroll_y);
    }

    ImGui::PopStyleVar(2);
  }
  ImGui::EndChild();
}

std::string AssemblyViewer::Decode(u8 op, u8 imm8, u16 imm16) const {
  switch (op) {
  case 0x00: return "NOP";
  case 0x01: return std::format("LD BC, ${:04X}", imm16);
  case 0x02: return "LD BC, A";
  case 0x03: return "INC BC";
  case 0x04: return "INC B";
  case 0x05: return "DEC B";
  case 0x06: return std::format("LD B, ${:02X}", imm8);
  case 0x07: return "RLCA";
  case 0x08: return std::format("LD (${:04X}), SP", imm16);
  case 0x09: return "ADD HL, BC";
  case 0x0A: return "LD A, (BC)";
  case 0x0B: return "DEC BC";
  case 0x0C: return "INC C";
  case 0x0D: return "DEC C";
  case 0x0E: return std::format("LD C, ${:02X}", imm8);
  case 0x0F: return "RRCA";
  case 0x10: return "STOP";
  case 0x11: return std::format("LD DE, ${:04X}", imm16);
  case 0x12: return "LD DE, (A)";
  case 0x13: return "INC DE";;
  case 0x14: return "INC D";
  case 0x15: return "DEC D";
  case 0x16: return std::format("LD D, ${:02X}", imm8);
  case 0x17: return "RLA";
  case 0x18: return std::format("JR ${:02X}", imm8);
  case 0x19: return "ADD HL, DE";
  case 0x1A: return "LD A, (DE)";
  case 0x1B: return "DEC DE";
  case 0x1C: return "INC E";
  case 0x1D: return "DEC E";
  case 0x1E: return std::format("LD E, ${:02X}", imm8);
  case 0x1F: return "RRA";
  case 0x20: return std::format("JR NZ, ${:02X}", imm8);
  case 0x21: return std::format("LD HL, ${:04X}", imm16);
  case 0x22: return "LD (HL+), A";
  case 0x23: return "INC HL";
  case 0x24: return "INC H";
  case 0x25: return "DEC H";
  case 0x26: return std::format("LD H, ${:02X}", imm8);
  case 0x27: return "DAA";
  case 0x28: return std::format("JR Z, ${:02X}", imm8);
  case 0x29: return "ADD HL, HL";
  case 0x2A: return "LD A, (HL)";
  case 0x2B: return "DEC HL";
  case 0x2C: return "INC L";
  case 0x2D: return "DEC L";
  case 0x2E: return std::format("LD L, ${:02X}", imm8);
  case 0x2F: return "CPL";
  case 0x30: return std::format("JR NC, ${:02X}", imm8);
  case 0x31: return std::format("LD SP, ${:04X}", imm16);
  case 0x32: return "LD (HL-), A";
  case 0x33: return "INC SP";
  case 0x34: return "INC (HL)";
  case 0x35: return "DEC (HL)";
  case 0x36: return std::format("LD (HL), ${:02X}", imm8);
  case 0x37: return "SCF";
  case 0x38: return std::format("JR C, ${:02X}", imm8);
  case 0x39: return "ADD HL, SP";
  case 0x3A: return "LD A, (HL-)";
  case 0x3B: return "DEC SP";
  case 0x3C: return "INC A";
  case 0x3D: return "DEC A";
  case 0x3E: return std::format("LD A, ${:02X}", imm8);
  case 0x3F: return "CCF";
  case 0x40: return "LD B, B";
  case 0x41: return "LD B, C";
  case 0x42: return "LD B, D";
  case 0x43: return "LD B, E";
  case 0x44: return "LD, B, H";
  case 0x45: return "LD B, L";
  case 0x46: return "LD B, (HL)";
  case 0x47: return "LD B, A";
  case 0x48: return "LD C, B";
  case 0x49: return "LD C, C";
  case 0x4A: return "LD C, D";
  case 0x4B: return "LD C, E";
  case 0x4C: return "LD C, H";
  case 0x4D: return "LD C, HL";
  case 0x4E: return "LD C, (HL)";
  case 0x4F: return "LD C, A";
  case 0x50: return "LD D, B";
  case 0x51: return "LD D, C";
  case 0x52: return "LD D, D";
  case 0x53: return "LD D, E";
  case 0x54: return "LD D, H";
  case 0x55: return "LD D, L";
  case 0x56: return "LD D, (HL)";
  case 0x57: return "LD D, A";
  case 0x58: return "LD E, B";
  case 0x59: return "LD E, C";
  case 0x5A: return "LD E, D";
  case 0x5B: return "LD E, E";
  case 0x5C: return "LD E, H";
  case 0x5D: return "LD E, L";
  case 0x5E: return "LD E, (HL)";
  case 0x5F: return "LD E, A";
  case 0x60: return "LD H, B";
  case 0x61: return "LD H, C";
  case 0x62: return "LD H, D";
  case 0x63: return "LD H, E";
  case 0x64: return "LD H, H";
  case 0x65: return "LD H, L";
  case 0x66: return "LD H, (HL)";
  case 0x67: return "LD H, A";
  case 0x68: return "LD L, B";
  case 0x69: return "LD L, C";
  case 0x6A: return "LD L, D";
  case 0x6B: return "LD L, E";
  case 0x6C: return "LD L, H";
  case 0x6D: return "LD L, L";
  case 0x6E: return "LD L, (HL)";
  case 0x6F: return "LD L, A";
  case 0x70: return "LD (HL), B";
  case 0x71: return "LD (HL), C";
  case 0x72: return "LD (HL), D";
  case 0x73: return "LD (HL), E";
  case 0x74: return "LD (HL), H";
  case 0x75: return "LD (HL), L";
  case 0x76: return "HALT";
  case 0x77: return "LD (HL), A";
  case 0x78: return "LD A, B";
  case 0x79: return "LD A, C";
  case 0x7A: return "LD A, D";
  case 0x7B: return "LD A, E";
  case 0x7C: return "LD A, H";
  case 0x7D: return "LD A, L";
  case 0x7E: return "LD A, (HL)";
  case 0x7F: return "LD A, A";
  case 0x80: return "ADD A, B";
  case 0x81: return "ADD A, C";
  case 0x82: return "ADD A, D";
  case 0x83: return "ADD A, E";
  case 0x84: return "ADD A, H";
  case 0x85: return "ADD A, L";
  case 0x86: return "ADD A, (HL)";
  case 0x87: return "ADD A, A";
  case 0x88: return "ADC A, B";
  case 0x89: return "ADC A, C";
  case 0x8A: return "ADC A, D";
  case 0x8B: return "ADC A, E";
  case 0x8C: return "ADC A, H";
  case 0x8D: return "ADC A, L";
  case 0x8E: return "ADC A, (HL)";
  case 0x8F: return "ADC A, A";
  case 0x90: return "SUB A, B";
  case 0x91: return "SUB A, C";
  case 0x92: return "SUB A, D";
  case 0x93: return "SUB A, E";
  case 0x94: return "SUB A, H";
  case 0x95: return "SUB A, L";
  case 0x96: return "SUB A, (HL)";
  case 0x97: return "SUB A, A";
  case 0x98: return "SBC A, B";
  case 0x99: return "SBC A, C";
  case 0x9A: return "SBC A, D";
  case 0x9B: return "SBC A, E";
  case 0x9C: return "SBC A, H";
  case 0x9D: return "SBC A, L";
  case 0x9E: return "SBC A, (HL)";
  case 0x9F: return "SBC A, A";
  case 0xA0: return "AND A, B";
  case 0xA1: return "AND A, C";
  case 0xA2: return "AND A, D";
  case 0xA3: return "AND A, E";
  case 0xA4: return "AND A, H";
  case 0xA5: return "AND A, L";
  case 0xA6: return "AND A, (HL)";
  case 0xA7: return "AND A, A";
  case 0xA8: return "XOR A, B";
  case 0xA9: return "XOR A, C";
  case 0xAA: return "XOR A, D";
  case 0xAB: return "XOR A, E";
  case 0xAC: return "XOR A, H";
  case 0xAD: return "XOR A, L";
  case 0xAE: return "XOR A, (HL)";
  case 0xAF: return "XOR A, A";
  case 0xB0: return "OR A, B";
  case 0xB1: return "OR A, C";
  case 0xB2: return "OR A, D";
  case 0xB3: return "OR A, E";
  case 0xB4: return "OR A, H";
  case 0xB5: return "OR A, L";
  case 0xB6: return "OR A, (HL)";
  case 0xB7: return "OR A, A";
  case 0xB8: return "CP A, B";
  case 0xB9: return "CP A, C";
  case 0xBA: return "CP A, D";
  case 0xBB: return "CP A, E";
  case 0xBC: return "CP A, H";
  case 0xBD: return "CP A, L";
  case 0xBE: return "CP A, (HL)";
  case 0xBF: return "CP A, A";
  case 0xC0: return "RET NZ";
  case 0xC1: return "POP BC";
  case 0xC2: return std::format("JP NZ, ${:04X}", imm16);
  case 0xC3: return std::format("JP ${:04X}", imm16);
  case 0xC4: return std::format("CALL NZ, ${:04X}", imm16);
  case 0xC5: return "PUSH BC";
  case 0xC6: return std::format("ADD A, ${:02X}", imm8);
  case 0xC7: return "RST 00h";
  case 0xC8: return "RET Z";
  case 0xC9: return "RET";
  case 0xCA: return "JP Z, ";
  case 0xCB: return "PREFIX CB";
  case 0xCC: return std::format("CALL Z, ${:02X}", imm16);
  case 0xCD: return std::format("CALL ${:04X}", imm16);
  case 0xCE: return std::format("ADC A, ${:02X}", imm8);
  case 0xCF: return "RST 08h";
  case 0xD0: return "RET NC";
  case 0xD1: return "POP DE";
  case 0xD2: return std::format("JP NC, ${:04X}", imm16);
  case 0xD4: return std::format("CALL NC, ${:04X}", imm16);
  case 0xD5: return "PUSH DE";
  case 0xD6: return std::format("SUB A, ${:02X}", imm8);
  case 0xD7: return "RST 10h";
  case 0xD8: return "RET C";
  case 0xD9: return "RETI";
  case 0xDA: return std::format("JP C, ${:04X}", imm16);
  case 0xDC: return std::format("CALL C, ${:04X}", imm16);
  case 0xDE: return std::format("SBC A, ${:02X}", imm8);
  case 0xDF: return "RST 18h";
  case 0xE0: return std::format("LD ($FF00+${:02X}), A", imm8);
  case 0xE1: return "POP HL";
  case 0xE2: return "LD ($FF00+C), A";
  case 0xE5: return "PUSH HL";
  case 0xE6: return std::format("AND A, ${:02X}", imm8);
  case 0xE7: return "RST 20h";
  case 0xE8: return std::format("ADD SP, ${:02X}", imm8);
  case 0xE9: return "JP HL";
  case 0xEA: return std::format("LD (${:04X}), A", imm16);
  case 0xEE: return std::format("XOR A, ${:02X}", imm8);
  case 0xEF: return "RST 28h";
  case 0xF0: return std::format("LD A, ($FF00+${:02X})", imm8);
  case 0xF1: return "POP AF";
  case 0xF2: return "LD A, ($FF00+C)";
  case 0xF3: return "DI";
  case 0xF5: return "PUSH AF";
  case 0xF6: return std::format("OR A, ${:02X}", imm8);
  case 0xF7: return "RST 30h";
  case 0xF8: return std::format("LD HL, SP+${:02X}", imm8);
  case 0xF9: return "LD SP, HL";
  case 0xFA: return std::format("LD A, (${:04X})", imm16);
  case 0xFB: return "EI";
  case 0xFE: return std::format("CP A, ${:02X}", imm8);
  case 0xFF: return "RST 38h";
  default:
    return "";
  }
}

std::string AssemblyViewer::DecodePrefixed(u8 op) const {
  switch (op) {
  case 0x00: return "RLC B";
  case 0x01: return "RLC C";
  case 0x02: return "RLC D";
  case 0x03: return "RLC E";
  case 0x04: return "RLC H";
  case 0x05: return "RLC L";
  case 0x06: return "RLC (HL)";
  case 0x07: return "RLC A";

  case 0x08: return "RRC B";
  case 0x09: return "RRC C";
  case 0x0a: return "RRC D";
  case 0x0b: return "RRC E";
  case 0x0c: return "RRC H";
  case 0x0d: return "RRC L";
  case 0x0e: return "RRC (HL)";
  case 0x0f: return "RRC A";

  case 0x10: return "RL B";
  case 0x11: return "RL C";
  case 0x12: return "RL D";
  case 0x13: return "RL E";
  case 0x14: return "RL H";
  case 0x15: return "RL L";
  case 0x16: return "RL (HL)";
  case 0x17: return "RL A";

  case 0x18: return "RR B";
  case 0x19: return "RR C";
  case 0x1a: return "RR D";
  case 0x1b: return "RR E";
  case 0x1c: return "RR H";
  case 0x1d: return "RR L";
  case 0x1e: return "RR (HL)";
  case 0x1f: return "RR A";

  case 0x20: return "SLA B";
  case 0x21: return "SLA C";
  case 0x22: return "SLA D";
  case 0x23: return "SLA E";
  case 0x24: return "SLA H";
  case 0x25: return "SLA L";
  case 0x26: return "SLA (HL)";
  case 0x27: return "SLA A";

  case 0x28: return "SRA B";
  case 0x29: return "SRA C";
  case 0x2a: return "SRA D";
  case 0x2b: return "SRA E";
  case 0x2c: return "SRA H";
  case 0x2d: return "SRA L";
  case 0x2e: return "SRA (HL)";
  case 0x2f: return "SRA A";

  case 0x30: return "SWAP B";
  case 0x31: return "SWAP C";
  case 0x32: return "SWAP D";
  case 0x33: return "SWAP E";
  case 0x34: return "SWAP H";
  case 0x35: return "SWAP L";
  case 0x36: return "SWAP (HL)";
  case 0x37: return "SWAP A";

  case 0x38: return "SRL B";
  case 0x39: return "SRL C";
  case 0x3a: return "SRL D";
  case 0x3b: return "SRL E";
  case 0x3c: return "SRL H";
  case 0x3d: return "SRL L";
  case 0x3e: return "SRL (HL)";
  case 0x3f: return "SRL A";

  case 0x40: return "BIT 0, B";
  case 0x41: return "BIT 0, C";
  case 0x42: return "BIT 0, D";
  case 0x43: return "BIT 0, E";
  case 0x44: return "BIT 0, H";
  case 0x45: return "BIT 0, L";
  case 0x46: return "BIT 0, (HL)";
  case 0x47: return "BIT 0, A";

  case 0x48: return "BIT 1, B";
  case 0x49: return "BIT 1, C";
  case 0x4a: return "BIT 1, D";
  case 0x4b: return "BIT 1, E";
  case 0x4c: return "BIT 1, H";
  case 0x4d: return "BIT 1, L";
  case 0x4e: return "BIT 1, (HL)";
  case 0x4f: return "BIT 1, A";

  case 0x50: return "BIT 2, B";
  case 0x51: return "BIT 2, C";
  case 0x52: return "BIT 2, D";
  case 0x53: return "BIT 2, E";
  case 0x54: return "BIT 2, H";
  case 0x55: return "BIT 2, L";
  case 0x56: return "BIT 2, (HL)";
  case 0x57: return "BIT 2, A";

  case 0x58: return "BIT 3, B";
  case 0x59: return "BIT 3, C";
  case 0x5a: return "BIT 3, D";
  case 0x5b: return "BIT 3, E";
  case 0x5c: return "BIT 3, H";
  case 0x5d: return "BIT 3, L";
  case 0x5e: return "BIT 3, (HL)";
  case 0x5f: return "BIT 3, A";

  case 0x60: return "BIT 4, B";
  case 0x61: return "BIT 4, C";
  case 0x62: return "BIT 4, D";
  case 0x63: return "BIT 4, E";
  case 0x64: return "BIT 4, H";
  case 0x65: return "BIT 4, L";
  case 0x66: return "BIT 4, (HL)";
  case 0x67: return "BIT 4, A";

  case 0x68: return "BIT 5, B";
  case 0x69: return "BIT 5, C";
  case 0x6a: return "BIT 5, D";
  case 0x6b: return "BIT 5, E";
  case 0x6c: return "BIT 5, H";
  case 0x6d: return "BIT 5, L";
  case 0x6e: return "BIT 5, (HL)";
  case 0x6f: return "BIT 5, A";

  case 0x70: return "BIT 6, B";
  case 0x71: return "BIT 6, C";
  case 0x72: return "BIT 6, D";
  case 0x73: return "BIT 6, E";
  case 0x74: return "BIT 6, H";
  case 0x75: return "BIT 6, L";
  case 0x76: return "BIT 6, (HL)";
  case 0x77: return "BIT 6, A";

  case 0x78: return "BIT 7, B";
  case 0x79: return "BIT 7, C";
  case 0x7a: return "BIT 7, D";
  case 0x7b: return "BIT 7, E";
  case 0x7c: return "BIT 7, H";
  case 0x7d: return "BIT 7, L";
  case 0x7e: return "BIT 7, (HL)";
  case 0x7f: return "BIT 7, A";

  case 0x80: return "RES 0, B";
  case 0x81: return "RES 0, C";
  case 0x82: return "RES 0, D";
  case 0x83: return "RES 0, E";
  case 0x84: return "RES 0, H";
  case 0x85: return "RES 0, L";
  case 0x86: return "RES 0, (HL)";
  case 0x87: return "RES 0, A";

  case 0x88: return "RES 1, B";
  case 0x89: return "RES 1, C";
  case 0x8a: return "RES 1, D";
  case 0x8b: return "RES 1, E";
  case 0x8c: return "RES 1, H";
  case 0x8d: return "RES 1, L";
  case 0x8e: return "RES 1, (HL)";
  case 0x8f: return "RES 1, A";

  case 0x90: return "RES 2, B";
  case 0x91: return "RES 2, C";
  case 0x92: return "RES 2, D";
  case 0x93: return "RES 2, E";
  case 0x94: return "RES 2, H";
  case 0x95: return "RES 2, L";
  case 0x96: return "RES 2, (HL)";
  case 0x97: return "RES 2, A";

  case 0x98: return "RES 3, B";
  case 0x99: return "RES 3, C";
  case 0x9a: return "RES 3, D";
  case 0x9b: return "RES 3, E";
  case 0x9c: return "RES 3, H";
  case 0x9d: return "RES 3, L";
  case 0x9e: return "RES 3, (HL)";
  case 0x9f: return "RES 3, A";

  case 0xa0: return "RES 4, B";
  case 0xa1: return "RES 4, C";
  case 0xa2: return "RES 4, D";
  case 0xa3: return "RES 4, E";
  case 0xa4: return "RES 4, H";
  case 0xa5: return "RES 4, L";
  case 0xa6: return "RES 4, (HL)";
  case 0xa7: return "RES 4, A";

  case 0xa8: return "RES 5, B";
  case 0xa9: return "RES 5, C";
  case 0xaa: return "RES 5, D";
  case 0xab: return "RES 5, E";
  case 0xac: return "RES 5, H";
  case 0xad: return "RES 5, L";
  case 0xae: return "RES 5, (HL)";
  case 0xaf: return "RES 5, A";

  case 0xb0: return "RES 6, B";
  case 0xb1: return "RES 6, C";
  case 0xb2: return "RES 6, D";
  case 0xb3: return "RES 6, E";
  case 0xb4: return "RES 6, H";
  case 0xb5: return "RES 6, L";
  case 0xb6: return "RES 6, (HL)";
  case 0xb7: return "RES 6, A";

  case 0xb8: return "RES 7, B";
  case 0xb9: return "RES 7, C";
  case 0xba: return "RES 7, D";
  case 0xbb: return "RES 7, E";
  case 0xbc: return "RES 7, H";
  case 0xbd: return "RES 7, L";
  case 0xbe: return "RES 7, (HL)";
  case 0xbf: return "RES 7, A";

  case 0xc0: return "SET 0, B";
  case 0xc1: return "SET 0, C";
  case 0xc2: return "SET 0, D";
  case 0xc3: return "SET 0, E";
  case 0xc4: return "SET 0, H";
  case 0xc5: return "SET 0, L";
  case 0xc6: return "SET 0, (HL)";
  case 0xc7: return "SET 0, A";

  case 0xc8: return "SET 1, B";
  case 0xc9: return "SET 1, C";
  case 0xca: return "SET 1, D";
  case 0xcb: return "SET 1, E";
  case 0xcc: return "SET 1, H";
  case 0xcd: return "SET 1, L";
  case 0xce: return "SET 1, (HL)";
  case 0xcf: return "SET 1, A";

  case 0xd0: return "SET 2, B";
  case 0xd1: return "SET 2, C";
  case 0xd2: return "SET 2, D";
  case 0xd3: return "SET 2, E";
  case 0xd4: return "SET 2, H";
  case 0xd5: return "SET 2, L";
  case 0xd6: return "SET 2, (HL)";
  case 0xd7: return "SET 2, A";

  case 0xd8: return "SET 3, B";
  case 0xd9: return "SET 3, C";
  case 0xda: return "SET 3, D";
  case 0xdb: return "SET 3, E";
  case 0xdc: return "SET 3, H";
  case 0xdd: return "SET 3, L";
  case 0xde: return "SET 3, (HL)";
  case 0xdf: return "SET 3, A";

  case 0xe0: return "SET 4, B";
  case 0xe1: return "SET 4, C";
  case 0xe2: return "SET 4, D";
  case 0xe3: return "SET 4, E";
  case 0xe4: return "SET 4, H";
  case 0xe5: return "SET 4, L";
  case 0xe6: return "SET 4, (HL)";
  case 0xe7: return "SET 4, A";

  case 0xe8: return "SET 5, B";
  case 0xe9: return "SET 5, C";
  case 0xea: return "SET 5, D";
  case 0xeb: return "SET 5, E";
  case 0xec: return "SET 5, H";
  case 0xed: return "SET 5, L";
  case 0xee: return "SET 5, (HL)";
  case 0xef: return "SET 5, A";

  case 0xf0: return "SET 6, B";
  case 0xf1: return "SET 6, C";
  case 0xf2: return "SET 6, D";
  case 0xf3: return "SET 6, E";
  case 0xf4: return "SET 6, H";
  case 0xf5: return "SET 6, L";
  case 0xf6: return "SET 6, (HL)";
  case 0xf7: return "SET 6, A";

  case 0xf8: return "SET 7, B";
  case 0xf9: return "SET 7, C";
  case 0xfa: return "SET 7, D";
  case 0xfb: return "SET 7, E";
  case 0xfc: return "SET 7, H";
  case 0xfd: return "SET 7, L";
  case 0xfe: return "SET 7, (HL)";
  case 0xff: return "SET 7, A";
  default: return "";
  }
}

std::string AssemblyViewer::GetInstruction(u16 addr) const {
  u8 op = emulator_->Read8(addr);
  u8 imm8 = emulator_->Read8(addr+1);
  u16 imm16 = emulator_->Read16(addr+1);
  return Decode(op, imm8, imm16);
}
