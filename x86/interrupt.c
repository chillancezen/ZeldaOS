/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <x86/include/interrupt.h>
#include <lib/include/string.h>
#include <x86/include/gdt.h>
#include <x86/include/ioport.h>
#include <kernel/include/printk.h>

struct interrupt_gate_entry IDT[IDT_SIZE] __attribute__((aligned(8)));
//one vector can not map to more than one device
//I.E. one device exclusively take one vector number.
int_handler *  handlers[IDT_SIZE];

void
dump_x86_cpustate(struct x86_cpustate *cpu)
{
    LOG_INFO("Dump cpu state: cpu(esp) 0x%x\n", cpu);
    printk("   edi:%x esi:%x ebp:%x\n", cpu->edi, cpu->esi, cpu->ebp);
    printk("   ebx:%x edx:%x ecx:%x eax:%x\n",
        cpu->ebx, cpu->edx, cpu->ecx, cpu->eax);
    printk("   gs:%x fs:%x, es:%x ds:%x\n",
        cpu->gs, cpu->fs, cpu->es, cpu->ds);
    printk("   vector:%x errorcode:%x eip:%x cs:%x\n",
        cpu->vector, cpu->errorcode, cpu->eip, cpu->cs);
    printk("   eflags:%x esp:%x ss:%x\n",
        cpu->eflags, cpu->esp, cpu->ss);
}
void
register_interrupt_handler(
    int vector_number,
    int_handler * handler,
    char * description)
{
    LOG_INFO("register interrupt handler:(%d, %s[0x%x])\n",
        vector_number,
        description,
        handler);
    handlers[vector_number] =  handler;
}
static void
set_interrupt_gate(int vector_number, void (*entry)(void))
{
    IDT[vector_number].offset_low = ((uint32_t)entry) & 0xffff;
    IDT[vector_number].selector = KERNEL_CODE_SELECTOR;
    IDT[vector_number].reserved0 = 0;
    IDT[vector_number].gate_size = GATE_SIZE_32;
    IDT[vector_number].DPL = DPL_0;
    IDT[vector_number].present = 1;
    IDT[vector_number].offset_high = (((uint32_t)entry) >> 16) & 0xffff;
}

uint32_t interrupt_handler(struct x86_cpustate * arg)
{
    uint32_t ESP = (uint32_t)arg;
    int_handler * device_interrup_handler = NULL;
    int vector = arg->vector;
    ASSERT(((vector >= 0) && (vector < IDT_SIZE)));
    device_interrup_handler = handlers[vector];
    if (!device_interrup_handler) {
        LOG_WARN("Can not find the interrup handler for vector:%d\n", vector);
    } else {
        ESP = device_interrup_handler(arg);
    }
    if (vector >= 40) {
        outb(PIC_SLAVE_COMMAND_PORT, 0x20);
    }
    outb(PIC_MASTER_COMMAND_PORT, 0x20);
    return ESP;
}

void idt_init(void)
{
    struct idt_pointer idtp;
    idtp.limit = sizeof(IDT) - 1;
    idtp.base = (uint32_t)(void*)IDT;

    memset(IDT, 0x0, sizeof(IDT));
    memset(handlers, 0x0, sizeof(handlers));
    set_interrupt_gate(0, int0);
    set_interrupt_gate(1, int1);
    set_interrupt_gate(2, int2);
    set_interrupt_gate(3, int3);
    set_interrupt_gate(4, int4);
    set_interrupt_gate(5, int5);
    set_interrupt_gate(6, int6);
    set_interrupt_gate(7, int7);
    set_interrupt_gate(8, int8);
    set_interrupt_gate(9, int9);
    set_interrupt_gate(10, int10);
    set_interrupt_gate(11, int11);
    set_interrupt_gate(12, int12);
    set_interrupt_gate(13, int13);
    set_interrupt_gate(14, int14);
    set_interrupt_gate(15, int15);
    set_interrupt_gate(16, int16);
    set_interrupt_gate(17, int17);
    set_interrupt_gate(18, int18);
    set_interrupt_gate(19, int19);
    set_interrupt_gate(20, int20);
    set_interrupt_gate(21, int21);
    set_interrupt_gate(22, int22);
    set_interrupt_gate(23, int23);
    set_interrupt_gate(24, int24);
    set_interrupt_gate(25, int25);
    set_interrupt_gate(26, int26);
    set_interrupt_gate(27, int27);
    set_interrupt_gate(28, int28);
    set_interrupt_gate(29, int29);
    set_interrupt_gate(30, int30);
    set_interrupt_gate(31, int31);
    set_interrupt_gate(32, int32);
    set_interrupt_gate(33, int33);
    set_interrupt_gate(34, int34);
    set_interrupt_gate(35, int35);
    set_interrupt_gate(36, int36);
    set_interrupt_gate(37, int37);
    set_interrupt_gate(38, int38);
    set_interrupt_gate(39, int39);
    set_interrupt_gate(40, int40);
    set_interrupt_gate(41, int41);
    set_interrupt_gate(42, int42);
    set_interrupt_gate(43, int43);
    set_interrupt_gate(44, int44);
    set_interrupt_gate(45, int45);
    set_interrupt_gate(46, int46);
    set_interrupt_gate(47, int47);
    set_interrupt_gate(48, int48);
    set_interrupt_gate(49, int49);
    set_interrupt_gate(50, int50);
    set_interrupt_gate(51, int51);
    set_interrupt_gate(52, int52);
    set_interrupt_gate(53, int53);
    set_interrupt_gate(54, int54);
    set_interrupt_gate(55, int55);
    set_interrupt_gate(56, int56);
    set_interrupt_gate(57, int57);
    set_interrupt_gate(58, int58);
    set_interrupt_gate(59, int59);
    set_interrupt_gate(60, int60);
    set_interrupt_gate(61, int61);
    set_interrupt_gate(62, int62);
    set_interrupt_gate(63, int63);
    set_interrupt_gate(64, int64);
    set_interrupt_gate(65, int65);
    set_interrupt_gate(66, int66);
    set_interrupt_gate(67, int67);
    set_interrupt_gate(68, int68);
    set_interrupt_gate(69, int69);
    set_interrupt_gate(70, int70);
    set_interrupt_gate(71, int71);
    set_interrupt_gate(72, int72);
    set_interrupt_gate(73, int73);
    set_interrupt_gate(74, int74);
    set_interrupt_gate(75, int75);
    set_interrupt_gate(76, int76);
    set_interrupt_gate(77, int77);
    set_interrupt_gate(78, int78);
    set_interrupt_gate(79, int79);
    set_interrupt_gate(80, int80);
    set_interrupt_gate(81, int81);
    set_interrupt_gate(82, int82);
    set_interrupt_gate(83, int83);
    set_interrupt_gate(84, int84);
    set_interrupt_gate(85, int85);
    set_interrupt_gate(86, int86);
    set_interrupt_gate(87, int87);
    set_interrupt_gate(88, int88);
    set_interrupt_gate(89, int89);
    set_interrupt_gate(90, int90);
    set_interrupt_gate(91, int91);
    set_interrupt_gate(92, int92);
    set_interrupt_gate(93, int93);
    set_interrupt_gate(94, int94);
    set_interrupt_gate(95, int95);
    set_interrupt_gate(96, int96);
    set_interrupt_gate(97, int97);
    set_interrupt_gate(98, int98);
    set_interrupt_gate(99, int99);
    set_interrupt_gate(100, int100);
    set_interrupt_gate(101, int101);
    set_interrupt_gate(102, int102);
    set_interrupt_gate(103, int103);
    set_interrupt_gate(104, int104);
    set_interrupt_gate(105, int105);
    set_interrupt_gate(106, int106);
    set_interrupt_gate(107, int107);
    set_interrupt_gate(108, int108);
    set_interrupt_gate(109, int109);
    set_interrupt_gate(110, int110);
    set_interrupt_gate(111, int111);
    set_interrupt_gate(112, int112);
    set_interrupt_gate(113, int113);
    set_interrupt_gate(114, int114);
    set_interrupt_gate(115, int115);
    set_interrupt_gate(116, int116);
    set_interrupt_gate(117, int117);
    set_interrupt_gate(118, int118);
    set_interrupt_gate(119, int119);
    set_interrupt_gate(120, int120);
    set_interrupt_gate(121, int121);
    set_interrupt_gate(122, int122);
    set_interrupt_gate(123, int123);
    set_interrupt_gate(124, int124);
    set_interrupt_gate(125, int125);
    set_interrupt_gate(126, int126);
    set_interrupt_gate(127, int127);
    set_interrupt_gate(128, int128);
    set_interrupt_gate(129, int129);
    set_interrupt_gate(130, int130);
    set_interrupt_gate(131, int131);
    set_interrupt_gate(132, int132);
    set_interrupt_gate(133, int133);
    set_interrupt_gate(134, int134);
    set_interrupt_gate(135, int135);
    set_interrupt_gate(136, int136);
    set_interrupt_gate(137, int137);
    set_interrupt_gate(138, int138);
    set_interrupt_gate(139, int139);
    set_interrupt_gate(140, int140);
    set_interrupt_gate(141, int141);
    set_interrupt_gate(142, int142);
    set_interrupt_gate(143, int143);
    set_interrupt_gate(144, int144);
    set_interrupt_gate(145, int145);
    set_interrupt_gate(146, int146);
    set_interrupt_gate(147, int147);
    set_interrupt_gate(148, int148);
    set_interrupt_gate(149, int149);
    set_interrupt_gate(150, int150);
    set_interrupt_gate(151, int151);
    set_interrupt_gate(152, int152);
    set_interrupt_gate(153, int153);
    set_interrupt_gate(154, int154);
    set_interrupt_gate(155, int155);
    set_interrupt_gate(156, int156);
    set_interrupt_gate(157, int157);
    set_interrupt_gate(158, int158);
    set_interrupt_gate(159, int159);
    set_interrupt_gate(160, int160);
    set_interrupt_gate(161, int161);
    set_interrupt_gate(162, int162);
    set_interrupt_gate(163, int163);
    set_interrupt_gate(164, int164);
    set_interrupt_gate(165, int165);
    set_interrupt_gate(166, int166);
    set_interrupt_gate(167, int167);
    set_interrupt_gate(168, int168);
    set_interrupt_gate(169, int169);
    set_interrupt_gate(170, int170);
    set_interrupt_gate(171, int171);
    set_interrupt_gate(172, int172);
    set_interrupt_gate(173, int173);
    set_interrupt_gate(174, int174);
    set_interrupt_gate(175, int175);
    set_interrupt_gate(176, int176);
    set_interrupt_gate(177, int177);
    set_interrupt_gate(178, int178);
    set_interrupt_gate(179, int179);
    set_interrupt_gate(180, int180);
    set_interrupt_gate(181, int181);
    set_interrupt_gate(182, int182);
    set_interrupt_gate(183, int183);
    set_interrupt_gate(184, int184);
    set_interrupt_gate(185, int185);
    set_interrupt_gate(186, int186);
    set_interrupt_gate(187, int187);
    set_interrupt_gate(188, int188);
    set_interrupt_gate(189, int189);
    set_interrupt_gate(190, int190);
    set_interrupt_gate(191, int191);
    set_interrupt_gate(192, int192);
    set_interrupt_gate(193, int193);
    set_interrupt_gate(194, int194);
    set_interrupt_gate(195, int195);
    set_interrupt_gate(196, int196);
    set_interrupt_gate(197, int197);
    set_interrupt_gate(198, int198);
    set_interrupt_gate(199, int199);
    set_interrupt_gate(200, int200);
    set_interrupt_gate(201, int201);
    set_interrupt_gate(202, int202);
    set_interrupt_gate(203, int203);
    set_interrupt_gate(204, int204);
    set_interrupt_gate(205, int205);
    set_interrupt_gate(206, int206);
    set_interrupt_gate(207, int207);
    set_interrupt_gate(208, int208);
    set_interrupt_gate(209, int209);
    set_interrupt_gate(210, int210);
    set_interrupt_gate(211, int211);
    set_interrupt_gate(212, int212);
    set_interrupt_gate(213, int213);
    set_interrupt_gate(214, int214);
    set_interrupt_gate(215, int215);
    set_interrupt_gate(216, int216);
    set_interrupt_gate(217, int217);
    set_interrupt_gate(218, int218);
    set_interrupt_gate(219, int219);
    set_interrupt_gate(220, int220);
    set_interrupt_gate(221, int221);
    set_interrupt_gate(222, int222);
    set_interrupt_gate(223, int223);
    set_interrupt_gate(224, int224);
    set_interrupt_gate(225, int225);
    set_interrupt_gate(226, int226);
    set_interrupt_gate(227, int227);
    set_interrupt_gate(228, int228);
    set_interrupt_gate(229, int229);
    set_interrupt_gate(230, int230);
    set_interrupt_gate(231, int231);
    set_interrupt_gate(232, int232);
    set_interrupt_gate(233, int233);
    set_interrupt_gate(234, int234);
    set_interrupt_gate(235, int235);
    set_interrupt_gate(236, int236);
    set_interrupt_gate(237, int237);
    set_interrupt_gate(238, int238);
    set_interrupt_gate(239, int239);
    set_interrupt_gate(240, int240);
    set_interrupt_gate(241, int241);
    set_interrupt_gate(242, int242);
    set_interrupt_gate(243, int243);
    set_interrupt_gate(244, int244);
    set_interrupt_gate(245, int245);
    set_interrupt_gate(246, int246);
    set_interrupt_gate(247, int247);
    set_interrupt_gate(248, int248);
    set_interrupt_gate(249, int249);
    set_interrupt_gate(250, int250);
    set_interrupt_gate(251, int251);
    set_interrupt_gate(252, int252);
    set_interrupt_gate(253, int253);
    set_interrupt_gate(254, int254);
    //remap the PIC
    outb(PIC_MASTER_DATA_PORT, 0xff);
    outb(PIC_SLAVE_DATA_PORT, 0xff);

    outb(PIC_MASTER_COMMAND_PORT, 0x11);
    outb(PIC_SLAVE_COMMAND_PORT, 0x11);

    outb(PIC_MASTER_DATA_PORT, 0x20);
    outb(PIC_SLAVE_DATA_PORT, 0x28);

    outb(PIC_MASTER_DATA_PORT, 0x04);
    outb(PIC_SLAVE_DATA_PORT, 0x02);

    outb(PIC_MASTER_DATA_PORT, 0x01);
    outb(PIC_SLAVE_DATA_PORT, 0x01);

    outb(PIC_MASTER_DATA_PORT, 0x00);// enable all
    outb(PIC_SLAVE_DATA_PORT, 0x00);
    __asm__ volatile("lidt %0;"
        :
        :"m"(idtp));
}
