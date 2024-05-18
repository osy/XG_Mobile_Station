/**
 * This is the reverse engineered ACPI functions dedicated to handling the eGPU.
 * Only relevant parts are included. This file should not be loaded.
 */
DefinitionBlock ("", "DSDT", 2, "ALASKA", "A M I ", 0x01072009)
{
    Scope (_SB)
    {
        // This is called to disconnect the dGPU/iGPU/eGPU
        Method (SUMA, 1, Serialized)
        {
            WOSR = One
            GPCE = One
            Local0 = DerefOf (EGIF [Zero])
            If ((Local0 == 0x80)){}
            ElseIf ((Local0 == 0x40))
            {
                If ((GPUM == One))
                {
                    Notify (^PCI0.GPP0.PEGP, 0x03) // Eject Request
                }
                ElseIf ((GPUV == One))
                {
                    Notify (^PCI0.GPP0.PEGP, 0x03) // Eject Request
                }
                ElseIf ((GPUV == 0x02))
                {
                    Notify (^PCI0.GPP0.SWUS.SWDS.VGA, 0x03) // Eject Request
                }
            }

            If ((Local0 == 0x40))
            {
                If ((WAT1 () == One))
                {
                    GPCE = Zero
                    Return (Zero)
                }
            }

            GPCE = Zero
            If ((Arg0 == One))
            {
                UMAF = One
            }

            Return (One)
        }

        Scope (ATKD)
        {
            // This is called from the AKS driver to query status and start the eGPU
            Method (WMNB, 3, Serialized)
            {
                CreateDWordField (Arg2, Zero, IIA0)
                CreateDWordField (Arg2, 0x04, IIA1)
                CreateDWordField (Arg2, 0x08, IIA2)
                CreateDWordField (Arg2, 0x0C, IIA3)
                CreateDWordField (Arg2, 0x10, IIA4)
                Local0 = (Arg1 & 0xFFFFFFFF)
                // ...
                If ((Local0 == 0x53545344)) // DSTS
                {
                    // ...
                    If ((IIA0 == 0x00090018)) // Get connector lock status
                    {
                        EGIF = ^^PCI0.SBRG.EC0.REBC (0x1C, 0x04)
                        Local1 = DerefOf (EGIF [One])
                        If ((Local1 & 0x02))
                        {
                            Return (0x00010001) // locked + valid
                        }
                        Else
                        {
                            Return (0x00010000) // unlocked + valid
                        }
                    }
                    // ...
                    If ((IIA0 == 0x00090019)) // Get eGPU connected
                    {
                        EGIF = ^^PCI0.SBRG.EC0.REBC (0x1C, 0x04)
                        Local1 = DerefOf (EGIF [Zero])
                        If ((Local1 & 0x40))
                        {
                            Return (0x00010001) // connected + valid
                        }
                        ElseIf ((Local1 & 0x80))
                        {
                            Return (0x00010000) // not connected + valid
                        }
                        Else
                        {
                            Return (Zero) // invalid
                        }
                    }
                    // ...
                    If ((IIA0 == 0x00090020)) // Get UMA mode
                    {
                        Local6 = (UMAF & One)
                        If ((Local6 == One))
                        {
                            Return (0x00010001)
                        }
                        ElseIf ((Local6 == Zero))
                        {
                            Return (0x00010000)
                        }
                        Else
                        {
                            Return (Zero)
                        }
                    }
                    // ...
                    If ((IIA0 == 0x0009001C)) // eGPU BIOS support
                    {
                        Return (0x00010003) // 0b1 = NVIDIA, 0b10 = AMD, 0b100 = Intel
                    }
                }
                // ...
                If ((Local0 == 0x53564544)) // DEVS
                {
                    // ...
                    If ((IIA0 == 0x00090019)) // Set eGPU connected
                    {
                        // Read pins from EC
                        EGIF = ^^PCI0.SBRG.EC0.REBC (0x1C, 0x04)
                        Local0 = DerefOf (EGIF [0x00]) // 0x80 = established, 0x40 = not established
                        Local1 = DerefOf (EGIF [0x01]) // 0x01 = connected, 0x02 = locked
                        Local2 = DerefOf (EGIF [0x02]) // 0x01 = AC power loss

                        // eGPU not connected
                        If ((Local0 == 0x80))
                        {
                            // AC power lost
                            If (((Local2 & 0x01) == 0x01))
                            {
                                Return (0x00)
                            }

                            // Cable connected + lock enabled
                            If (((Local1 & 0x03) != 0x03))
                            {
                                Return (0x00)
                            }
                        }

                        Local6 = (UMAF & 0x01)
                        If ((Local6 == 0x00))
                        {
                            If (ATKP)
                            {
                                // Notify ATK
                                IANE (0xBE)
                            }

                            If ((SUMA (0x01) == 0x00)) // disable dGPU
                            {
                                Return (0x01) // success
                            }

                            If (ATKP)
                            {
                                // Notify ATK: BIOS restart required
                                IANE (0xC2)
                            }
                        }

                        Sleep (0x03E8)

                        If ((IIA1 == 0x00)) // off
                        {
                            EGIF [0x00] = 0x0A
                            GPUM = 0x01
                        }
                        ElseIf ((IIA1 != 0x00)) // on
                        {
                            EGIF [0x00] = 0x0B
                            GPUM = 0x02
                            If ((Local6 == 0x01))
                            {
                                UMAF = 0x02
                            }
                        }

                        Local1 = M017 (0x00, 0x01, 0x01, 0x88, 0x00, 0x08)
                        Local1 &= 0xF0
                        If ((IIA1 == 0x0000)) // iGPU
                        {
                            GPUV = 0x01
                            M018 (0x00, 0x01, 0x01, 0x88, 0x00, 0x08, (Local1 | 0x03))
                        }
                        ElseIf ((IIA1 == 0x0101)) // AMD eGPU
                        {
                            GPUV = 0x02
                            M018 (0x00, 0x01, 0x01, 0x88, 0x00, 0x08, (Local1 | 0x03))
                        }
                        ElseIf ((IIA1 == 0x0201)) // Intel eGPU
                        {
                            GPUV = 0x03
                            M018 (0x00, 0x01, 0x01, 0x88, 0x00, 0x08, (Local1 | 0x03))
                        }
                        Else // NVIDIA eGPU
                        {
                            GPUV = 0x00
                            M018 (0x00, 0x01, 0x01, 0x88, 0x00, 0x08, (Local1 | 0x04))
                        }

                        // Write to EC
                        ^^PCI0.SBRG.EC0.WEBC (0x1C, 0x01, EGIF)
                        // Wait for write to complete
                        WAT3 (Local0)
                        CONT += 0x01

                        If ((IIA1 == 0x00)){} // off
                        ElseIf ((IIA1 != 0x00)) // on
                        {
                            // Turn on eGPU hardware support
                            FGON ()
                            Sleep (2000)
                            // Notify ACPI changes
                            If ((SHGM (0x02) == 0x00)) // if PCI device is not detected
                            {
                                Return (0x02) // reboot required
                            }

                            If ((GPUV == 0x02)) // AMD only
                            {
                                If ((ACPT == 0x01))
                                {
                                    Local5 = 0x64
                                }
                                ElseIf ((ACPT == 0x02))
                                {
                                    Local5 = 0xA5
                                }
                                ElseIf ((ACPT == 0x03))
                                {
                                    Local5 = 0x50
                                }
                                Else
                                {
                                    Local5 = 0x64
                                }

                                Local5 <<= 0x08
                                ^^PCI0.GP17.VGA.AFNC (0x02, Local5)
                            }
                        }

                        Return (0x01) // success
                    }
                }
            }
        }
    }

    Scope (PCI0)
    {
        Scope (SBRG)
        {
            Scope (EC0)
            {
                // This is called when connector is mated/unmated and lock detect is asserted/deasserted
                Method (_QF5, 0, Serialized)  // _Qxx: EC Query, xx=0x00-0xFF
                {
                    Previous = DerefOf (EGIF [0x01])
                    EGIF = REBC (0x1C, 0x04)
                    Local0 = DerefOf (EGIF [0x00])
                    Local1 = DerefOf (EGIF [0x01])
                    Local2 = DerefOf (EGIF [0x02])
                    Switch (Previous)
                    {
                        Case (0x00) // not connected
                        {
                            If ((Local1 == 0x01)) // connected
                            {
                                If (ATKP)
                                {
                                    ^^^^ATKD.IANE (0xB9) // Connect Change
                                }
                            }
                        }
                        Case (0x01) // connected
                        {
                            If ((Local1 == 0x00)) // not connected
                            {
                                If (ATKP)
                                {
                                    ^^^^ATKD.IANE (0xB9) // Connect Change
                                }
                            }
                
                            If ((Local1 == 0x03)) // locked
                            {
                                If (ATKP)
                                {
                                    ^^^^ATKD.IANE (0xBA) // Switch-Lock Change
                                }
                
                                If ((Local0 == 0x40)) // eGPU enabled
                                {
                                    Return (0x00)
                                }
                            }
                        }
                        Case (0x03) // locked
                        {
                            If ((Local1 == 0x01)) // unlocked
                            {
                                If (ATKP)
                                {
                                    ^^^^ATKD.IANE (0xBA) // Switch-Lock Change
                                }
                            }
                        }
                    }

                    // NOTE: there is a bug here where if the lock switch is asserted at the
                    // same time as the connector is mated, it would not be detected because
                    // there is no going from 0x00 -> 0x03.
                
                    If ((Local0 == 0x40)) // eGPU connected (but cable no longer locked)
                    {
                        If (ATKP)
                        {
                            ^^^^ATKD.IANE (0xBE)
                        }
                
                        If ((SUMA (0x00) == 0x00)) // disable eGPU
                        {
                            Return (0x00)
                        }
                
                        If (ATKP)
                        {
                            ^^^^ATKD.IANE (0xC2) // restart required
                        }
                
                        Sleep (0x03E8)
                        EGIF [0x00] = 0x0A // turn off
                        GPUM = 0x01
                        GPUV = 0x00
                        Local1 = M017 (0x00, 0x01, 0x01, 0x88, 0x00, 0x08)
                        Local1 &= 0xF0
                        M018 (0x00, 0x01, 0x01, 0x88, 0x00, 0x08, (Local1 | 0x04))
                        WEBC (0x1C, 0x01, EGIF) // write to EC
                        WAT3 (Local0) // wait for EC write to complete
                        Local6 = (UMAF & 0x02)
                        If ((Local6 == 0x00))
                        {
                            FGON ()
                            Sleep (0x07D0)
                            If ((SHGM (0x01) == 0x00))
                            {
                                Return (0x02)
                            }
                        }
                        Else
                        {
                            UMAF = 0x01
                        }
                    }
                    Else
                    {
                        Return (0x00)
                    }
                }

                // This is called when P_AC_LOSS_10 changes
                Method (_QF6, 0, NotSerialized)  // _Qxx: EC Query, xx=0x00-0xFF
                {
                    EGIF = REBC (0x1C, 0x04)
                    Local0 = DerefOf (EGIF [0x00])
                    Local1 = DerefOf (EGIF [0x01])
                    Local2 = DerefOf (EGIF [0x02])
                    If (((Local2 & 0x01) == 0x01))
                    {
                        If (ATKP)
                        {
                            ^^^^ATKD.IANE (0xBB) // External Power Loss
                        }
                    }

                    If ((Local0 == 0x40)) // eGPU connected, same as _QF5
                    {
                        If (ATKP)
                        {
                            ^^^^ATKD.IANE (0xBE)
                        }

                        If ((SUMA (0x00) == 0x00))
                        {
                            Return (0x00)
                        }

                        If (ATKP)
                        {
                            ^^^^ATKD.IANE (0xC2)
                        }

                        Sleep (0x03E8)
                        EGIF [0x00] = 0x0A
                        GPUM = 0x01
                        GPUV = 0x00
                        Local1 = M017 (0x00, 0x01, 0x01, 0x88, 0x00, 0x08)
                        Local1 &= 0xF0
                        M018 (0x00, 0x01, 0x01, 0x88, 0x00, 0x08, (Local1 | 0x04))
                        WEBC (0x1C, 0x01, EGIF)
                        WAT3 (Local0)
                        Local6 = (UMAF & 0x02)
                        If ((Local6 == 0x00))
                        {
                            FGON ()
                            Sleep (0x07D0)
                            If ((SHGM (0x01) == 0x00))
                            {
                                Return (0x02)
                            }
                        }
                        Else
                        {
                            UMAF = 0x01
                        }
                    }
                    Else
                    {
                        Return (0x00)
                    }
                }
            }
        }
    }
}
