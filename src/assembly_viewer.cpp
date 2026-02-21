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
        auto addr = static_cast<uint16_t>(i);
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

std::string AssemblyViewer::Decode(uint8_t op) const {
  switch (op) {
  case 0x00: return "NOP";
  case 0x01: return std::format("LD BC, ");
    // return {Opcode::LD, 3, 12, 12, Operands_Reg16_Imm16{ Reg16::BC }};
  case 0x02: return "LD BC, A";
  case 0x03: return "INC BC";
  case 0x04: return "INC B";
  case 0x05: return "DEC B";
  case 0x06: return "LD B, ";
    // return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::B}};
  case 0x07: return "RLCA";
  case 0x08: return "LD (), SP";
    // return {Opcode::LD, 3, 20, 20, Operands_Imm16_Ptr_SP{}};
  case 0x09: return "ADD HL, BC";
  case 0x0A: return "LD A, (BC)";
  case 0x0B: return "DEC BC";
  case 0x0C: return "INC C";
  case 0x0D: return "DEC C";
  case 0x0E: return "LD C, ()";
    // return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::C}};
  case 0x0F: return "RRCA";
  case 0x10: return "STOP";
  case 0x11: return "LD DE, ";
    // return {Opcode::LD, 3, 12, 12, Operands_Reg16_Imm16{Reg16::DE}};
  case 0x12: return "LD DE, (A)";
  case 0x13: return "INC DE";;
  case 0x14: return "INC D";
  case 0x15: return "DEC D";
  case 0x16: return "LD D, ";
    // return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::D}};
  case 0x17: return "RLA";
  case 0x18: return "JR ";
    // return {Opcode::JR, 2, 12, 12, Operands_Offset{}};
  case 0x19: return "ADD HL, DE";
  case 0x1A: return "LD A, (DE)";
  case 0x1B: return "DEC DE";
  case 0x1C: return "INC E";
  case 0x1D: return "DEC E";
  case 0x1E: return "LD E, ";
    // return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::E}};
  case 0x1F: return "RRA";
  case 0x20: return "JR NZ, ";
    // return {Opcode::JR, 2, 12, 8, Operands_Cond_Offset{Cond::NZ}};
  case 0x21: return "LD HL, ";
    // return {Opcode::LD, 3, 12, 12, Operands_Reg16_Imm16{Reg16::HL}};
  case 0x22: return "LD (HL), A";
    // return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Inc_Reg8{Reg16::HL, Reg8::A}};
  case 0x23: return "INC HL";
  case 0x24: return "INC H";
  case 0x25: return "DEC H";
  case 0x26: return "LD H, ";
    // return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::H}};
  case 0x27: return "DAA";
  case 0x28: return "JR Z, ";
    // return {Opcode::JR, 2, 12, 8, Operands_Cond_Offset{Cond::Z}};
  case 0x29: return "ADD HL, HL";
  case 0x2A: return "LD A, (HL)";
  case 0x2B: return "DEC HL";
  case 0x2C: return "INC L";
  case 0x2D: return "DEC L";
  case 0x2E: return "LD L, ";
    // return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::L}};
  case 0x2F: return "CPL";
  case 0x30: return "JR NC, ";
    // return {Opcode::JR, 2, 12, 8, Operands_Cond_Offset{Cond::NC}};
  case 0x31: return "LD SP, ";
    // return {Opcode::LD, 3, 12, 12, Operands_SP_Imm16{}};
  case 0x32: return "LD (HL-), A";
  case 0x33: return "INC SP";
  case 0x34: return "INC (HL)";
  case 0x35: return "DEC (HL)";
  case 0x36: return "LD (HL), ";
    // return {Opcode::LD, 2, 12, 12, Operands_Reg16_Ptr_Imm8{Reg16::HL}};
  case 0x37: return "SCF";
  case 0x38: return "JR C, ";
    // return {Opcode::JR, 2, 12, 8, Operands_Cond_Offset{Cond::C}};
  case 0x39: return "ADD HL, SP";
  case 0x3A: return "LD A, (HL--)";
  case 0x3B: return "DEC SP";
  case 0x3C: return "INC A";
  case 0x3D: return "DEC A";
  case 0x3E: return "LD A, ";
    // return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::A}};
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
  case 0xC2: return "JP NZ, ";
    // return {Opcode::JP, 3, 16, 12, Operands_Cond_Imm16{Cond::NZ}};
  case 0xC3: return "JP ";
    // return {Opcode::JP, 3, 16, 16, Operands_Imm16{}};
  case 0xC4: return "CALL NZ, ";
    // return {Opcode::CALL, 3, 24, 12, Operands_Cond_Imm16{Cond::NZ}};
  case 0xC5: return "PUSH BC";
  case 0xC6: return "ADD A, ";
    // return {Opcode::ADD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::A}};
  case 0xC7: return "RST 00h";
  case 0xC8: return "RET Z";
  case 0xC9: return "RET";
  case 0xCA: return "JP Z, ";
  case 0xCB: return "PREFIX CB";
  case 0xCC: return "CALL Z, ";
    // return {Opcode::CALL, 3, 24, 12, Operands_Cond_Imm16{Cond::Z}};
  case 0xCD: return "CALL ";
    // return {Opcode::CALL, 3, 24, 24, Operands_Imm16{}};
  case 0xCE: return "ADC A, ";
    // return {Opcode::ADC, 2, 8, 8, Operands_Reg8_Imm8{Reg8::A}};
  case 0xCF: return "RST 08h";
  case 0xD0: return "RET NC";
  case 0xD1: return "POP DE";
  case 0xD2: return "JP NC, ";
    // return {Opcode::JP, 3, 16, 12, Operands_Cond_Imm16{Cond::NC}};
  case 0xD4: return "CALL NC, ";
    // return {Opcode::CALL, 3, 24, 12, Operands_Cond_Imm16{Cond::NC}};
  case 0xD5: return "PUSH DE";
  case 0xD6: return "SUB A, ";
    // return {Opcode::SUB, 2, 8, 8, Operands_Imm8{}};
  case 0xD7: return "RST 10h";
  case 0xD8: return "RET C";
  case 0xD9: return "RETI";
  case 0xDA: return "JP C, ";
    // return {Opcode::JP, 3, 16, 12, Operands_Cond_Imm16{Cond::C}};
  case 0xDC: return "CALL C, ";
    // return {Opcode::CALL, 3, 24, 12, Operands_Cond_Imm16{Cond::C}};
  case 0xDE: return "SBC A, ";
    // return {Opcode::SBC, 2, 8, 8, Operands_Reg8_Imm8{Reg8::A}};
  case 0xDF: return "RST 18h";
  case 0xE0: return "LD ($FF00+), A";
    // return {Opcode::LDH, 2, 12, 12, Operands_Imm8_Ptr_Reg8{0, Reg8::A}};
  case 0xE1: return "POP HL";
  case 0xE2: return "LD ($FF00+C), A";
  case 0xE5: return "PUSH HL";
  case 0xE6: return "AND A, ";
    // return {Opcode::AND, 2, 8, 8, Operands_Imm8{}};
  case 0xE7: return "RST 20h";
  case 0xE8: return "ADD SP, ";
    // return {Opcode::ADD, 2, 16, 16, Operands_SP_Offset{}};
  case 0xE9: return "JP HL";
  case 0xEA: return "LD (), A";
    // return {Opcode::LD, 3, 16, 16, Operands_Imm16_Ptr_Reg8{0, Reg8::A}};
  case 0xEE: return "XOR A, ";
    // return {Opcode::XOR, 2, 8, 8, Operands_Imm8{}};
  case 0xEF: return "RST 28h";
  case 0xF0: return "LD ($FF00+), A";
    // return {Opcode::LDH, 2, 12, 12, Operands_Reg8_Imm8_Ptr{Reg8::A}};
  case 0xF1: return "POP AF";
  case 0xF2: return "LD A, ($FF00+C)";
  case 0xF3: return "DI";
  case 0xF5: return "PUSH AF";
  case 0xF6: return "OR A, ";
    // return {Opcode::OR, 2, 8, 8, Operands_Imm8{}};
  case 0xF7: return "RST 30h";
  case 0xF8: return "LD HL, SP+";
    // return {Opcode::LD, 2, 12, 12, Operands_Reg16_SP_Offset{Reg16::HL}};
  case 0xF9: return "LD SP, HL";
  case 0xFA: return "LD A, ()";
    // return {Opcode::LD, 3, 16, 16, Operands_Reg8_Imm16_Ptr{Reg8::A, 0}};
  case 0xFB: return "EI";
  case 0xFE: return "CP A, ";
    // return {Opcode::CP, 2, 8, 8, Operands_Imm8{}};
  case 0xFF: return "RST 38h";
  default:
    return "";
  }
}

std::string AssemblyViewer::DecodePrefixed(uint8_t op) const {
  // int r8 = op & 0x7;

  // uint8_t bytes = 2;
  // uint8_t cycles = r8 == 6 ? 16 : 8;

  // if (op & 0xC0) {
  //   uint8_t b3 = (op & 0x38) >> 3;
  //   auto operands = ([=]() -> Operands {
  //     switch (r8) {
  //     case 0:
  //       return Operands_Imm8_Literal_Reg8{b3, Reg8::B};
  //     case 1:
  //       return Operands_Imm8_Literal_Reg8{b3, Reg8::C};
  //     case 2:
  //       return Operands_Imm8_Literal_Reg8{b3, Reg8::D};
  //     case 3:
  //       return Operands_Imm8_Literal_Reg8{b3, Reg8::E};
  //     case 4:
  //       return Operands_Imm8_Literal_Reg8{b3,Reg8::H};
  //     case 5:
  //       return Operands_Imm8_Literal_Reg8{b3, Reg8::L};
  //     case 6:
  //       return Operands_Imm8_Literal_Reg16_Ptr{b3, Reg16::HL};
  //     case 7:
  //       return Operands_Imm8_Literal_Reg8{b3, Reg8::A};
  //     default:
  //       std::unreachable();
  //     }
  //   })();

  //   switch ((op & 0xC0) >> 6) {
  //   case 1: {
  //     cycles = r8 == 6 ? 12 : 8;
  //     return {Opcode::BIT, bytes, cycles, cycles, operands};
  //   }
  //   case 2:
  //     return {Opcode::RES, bytes, cycles, cycles, operands};
  //   case 3:
  //     return {Opcode::SET, bytes, cycles, cycles, operands};
  //   default:
  //     return {Opcode::INVALID, 1, 4, 4, Operands_None{}};
  //   }
  return "";
}

std::string AssemblyViewer::GetInstruction(uint16_t addr) const {
  uint8_t op = emulator_->Read8(addr);
  return Decode(op);
}
