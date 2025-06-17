library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_textio.all;
use ieee.std_logic_unsigned.all;
use std.textio.all;
use ieee.numeric_std.all;

entity tb is
    generic(
        address_width: integer := 15;
        memory_file : string := "code.txt";
        log_file: string := "out.txt";
        uart_support : string := "no"
    );
end tb;

architecture tb of tb is
    signal clock_in, reset, data, stall, stall_sig: std_logic := '0';
    signal uart_read, uart_write: std_logic;
    signal boot_enable_n, ram_enable_n, ram_dly: std_logic;
    signal address, data_read, data_write, data_read_boot, data_read_ram: std_logic_vector(31 downto 0);
    signal ext_irq: std_logic_vector(7 downto 0);
    signal data_we, data_w_n_ram: std_logic_vector(3 downto 0);

    signal periph, periph_dly, periph_wr, periph_irq: std_logic;
    signal data_read_periph, data_read_periph_s, data_write_periph: std_logic_vector(31 downto 0);
    signal gpioa_in, gpioa_out, gpioa_ddr: std_logic_vector(15 downto 0);
    signal gpiob_in, gpiob_out, gpiob_ddr: std_logic_vector(15 downto 0);
    signal gpio_sig, gpio_sig2, gpio_sig3: std_logic := '0';
    
    -- Signals for 3DES Core Integration
    signal ext_periph, ext_periph_dly: std_logic;
    signal data_in_des, key1_in_des, key2_in_des, key3_in_des, data_out_des: std_logic_vector(0 to 63) := (others => '0');
    signal function_select_des, lddata_des, ldkey_des, reset_des, out_ready_des: std_logic := '0';
    signal data_read_des, data_read_des_s: std_logic_vector(31 downto 0) := (others => '0');

begin

    process                     --25Mhz system clock
    begin
        clock_in <= not clock_in;
        wait for 20 ns;
        clock_in <= not clock_in;
        wait for 20 ns;
    end process;

    process
    begin
        wait for 4 ms;
        gpio_sig <= not gpio_sig;
        gpio_sig2 <= not gpio_sig2;
        wait for 100 us;
        gpio_sig <= not gpio_sig;
        gpio_sig2 <= not gpio_sig2;
    end process;

    process
    begin
        wait for 5 ms;
        gpio_sig3 <= not gpio_sig3;
        wait for 5 ms;
        gpio_sig3 <= not gpio_sig3;
    end process;

    gpioa_in <= x"00" & "0000" & gpio_sig & "000";
    gpiob_in <= "10000" & gpio_sig3 & "00" & "00000" & gpio_sig2 & "00";

    process
    begin
        stall <= not stall;
        wait for 123 ns;
        stall <= not stall;
        wait for 123 ns;
    end process;

    reset <= '0', '1' after 5 ns, '0' after 500 ns;
    stall_sig <= '0'; --stall;
    ext_irq <= "0000000" & periph_irq;

    boot_enable_n <= '0' when (address(31 downto 28) = "0000" and stall_sig = '0') or reset = '1' else '1';
    ram_enable_n <= '0' when (address(31 downto 28) = "0100" and stall_sig = '0') or reset = '1' else '1';
    data_read <= data_read_des when ext_periph = '1' or ext_periph_dly = '1' else data_read_periph when periph = '1' or periph_dly = '1' else data_read_boot when address(31 downto 28) = "0000" and ram_dly = '0' else data_read_ram;
    data_w_n_ram <= not data_we;

    process(clock_in, reset)
    begin
        if reset = '1' then
            ram_dly <= '0';
            periph_dly <= '0';
            ext_periph_dly <= '0';
        elsif clock_in'event and clock_in = '1' then
            ram_dly <= not ram_enable_n;
            periph_dly <= periph;
            ext_periph_dly <= ext_periph;
        end if;
    end process;

    -- HF-RISCV core
    processor: entity work.processor
    port map(   clk_i => clock_in,
            rst_i => reset,
            stall_i => stall_sig,
            addr_o => address,
            data_i => data_read,
            data_o => data_write,
            data_w_o => data_we,
            data_mode_o => open,
            extio_in => ext_irq,
            extio_out => open
    );

    data_read_periph <= data_read_periph_s(7 downto 0) & data_read_periph_s(15 downto 8) & data_read_periph_s(23 downto 16) & data_read_periph_s(31 downto 24);
    data_write_periph <= data_write(7 downto 0) & data_write(15 downto 8) & data_write(23 downto 16) & data_write(31 downto 24);
    periph_wr <= '1' when data_we /= "0000" else '0';
    periph <= '1' when address(31 downto 24) = x"e1" else '0';

    peripherals: entity work.peripherals
    port map(
        clk_i => clock_in,
        rst_i => reset,
        addr_i => address,
        data_i => data_write_periph,
        data_o => data_read_periph_s,
        sel_i => periph,
        wr_i => periph_wr,
        irq_o => periph_irq,
        gpioa_in => gpioa_in,
        gpioa_out => gpioa_out,
        gpioa_ddr => gpioa_ddr,
        gpiob_in => gpiob_in,
        gpiob_out => gpiob_out,
        gpiob_ddr => gpiob_ddr
    );
    
    -- 3DES Core
    data_read_des <= data_read_des_s(7 downto 0) & data_read_des_s(15 downto 8) & data_read_des_s(23 downto 16) & data_read_des_s(31 downto 24);
    ext_periph <= '1' when address(31 downto 24) = x"e7" else '0';
    
    -- Read Process: Maps 3DES outputs to processor's readable addresses
    process (clock_in, reset, address, data_out_des, function_select_des, reset_des, ldkey_des, lddata_des, out_ready_des)
    begin
        if reset = '1' then
            data_read_des_s <= (others => '0');
        elsif clock_in'event and clock_in = '1' then
            if (ext_periph = '1') then  -- 3DES at 0xe7000000
                case address(7 downto 4) is
                    when "0000" =>      -- Control Register Read: 0xe7000000
                                        -- Provides status of control signals, especially out_ready at bit 0
                        data_read_des_s <= x"000000" & "000" & function_select_des & reset_des & ldkey_des & lddata_des & out_ready_des;
                    
                    when "0001" =>      -- Not readable, write-only
                        data_read_des_s <= (others => '0');
                    when "0010" =>      -- Not readable, write-only
                        data_read_des_s <= (others => '0');
                    when "0011" =>      -- Not readable, write-only
                        data_read_des_s <= (others => '0');
                    when "0100" =>      -- Not readable, write-only
                        data_read_des_s <= (others => '0');
                    when "0101" =>      -- Not readable, write-only
                        data_read_des_s <= (others => '0');
                    when "0110" =>      -- Not readable, write-only
                        data_read_des_s <= (others => '0');
                    when "0111" =>      -- Not readable, write-only
                        data_read_des_s <= (others => '0');
                    when "1000" =>      -- Not readable, write-only
                        data_read_des_s <= (others => '0');
                        
                    when "1001" =>      -- Output Data Read [0]: 0xe7000090
                        data_read_des_s <= data_out_des(32 to 63);
                    when "1010" =>      -- Output Data Read [1]: 0xe70000A0
                        data_read_des_s <= data_out_des(0 to 31);
                    when others =>
                        data_read_des_s <= (others => '0');
                end case;
            end if;
        end if;
    end process;

    -- Write Process: Maps processor writes to 3DES inputs
    process (clock_in, reset)
    begin
        if reset = '1' then
            key1_in_des <= (others => '0');
            key2_in_des <= (others => '0');
            key3_in_des <= (others => '0');
            data_in_des <= (others => '0');
            function_select_des <= '0';
            reset_des <= '0';
            ldkey_des <= '0';
            lddata_des <= '0';
        elsif clock_in'event and clock_in = '1' then
            if (ext_periph = '1' and data_we /= "0000") then    -- A write occurs in the 3DES address space
                case address(7 downto 4) is
                    when "0000" =>      -- Control Register Write: 0xe7000000
                                        -- Maps bits from the C driver to control signals
                         function_select_des <= data_write_periph(4);
                         reset_des <= data_write_periph(3);
                         ldkey_des <= data_write_periph(2);
                         lddata_des <= data_write_periph(1);
                         
                    when "0001" =>      -- Key 1 Write [0]: 0xe7000010
                        key1_in_des(32 to 63) <= data_write_periph;
                    when "0010" =>      -- Key 1 Write [1]: 0xe7000020
                        key1_in_des(0 to 31) <= data_write_periph;  
                        
                    when "0011" =>      -- Key 2 Write [0]: 0xe7000030
                        key2_in_des(32 to 63) <= data_write_periph;
                    when "0100" =>      -- Key 2 Write [1]: 0xe7000040
                        key2_in_des(0 to 31) <= data_write_periph;
                        
                    when "0101" =>      -- Key 3 Write [0]: 0xe7000050
                        key3_in_des(32 to 63) <= data_write_periph;
                    when "0110" =>      -- Key 3 Write [1]: 0xe7000060
                        key3_in_des(0 to 31) <= data_write_periph;
                        
                    when "0111" =>      -- Input Data Write [0]: 0xe7000070
                        data_in_des(32 to 63) <= data_write_periph;
                    when "1000" =>      -- Input Data Write [1]: 0xe7000080
                        data_in_des(0 to 31) <= data_write_periph;
                    when others =>
                        NULL;
                end case;
            end if;
        end if;
    end process;
    
    -- 3DES Core Instantiation
    crypto_core: entity work.tdes_top
    port map(   
        key1_in => key1_in_des,
        key2_in => key2_in_des,
        key3_in => key3_in_des,
        function_select => function_select_des, -- From control register
        data_in  => data_in_des,
        data_out => data_out_des,
        lddata => lddata_des,                   -- From control register
        ldkey => ldkey_des,                     -- From control register
        out_ready => out_ready_des,             -- To control register
        reset => reset_des,                     -- From control register
        clock => clock_in
    );

    -- boot ROM
    boot0lb: entity work.boot_ram
    generic map (   memory_file => "boot.txt",
                    data_width => 8,
                    address_width => 12,
                    bank => 0)
    port map(
        clk     => clock_in,
        addr    => address(11 downto 2),
        cs_n    => boot_enable_n,
        we_n    => '1',
        data_i  => (others => '0'),
        data_o  => data_read_boot(7 downto 0)
    );

    boot0ub: entity work.boot_ram
    generic map (   memory_file => "boot.txt",
                    data_width => 8,
                    address_width => 12,
                    bank => 1)
    port map(
        clk     => clock_in,
        addr    => address(11 downto 2),
        cs_n    => boot_enable_n,
        we_n    => '1',
        data_i  => (others => '0'),
        data_o  => data_read_boot(15 downto 8)
    );

    boot1lb: entity work.boot_ram
    generic map (   memory_file => "boot.txt",
                    data_width => 8,
                    address_width => 12,
                    bank => 2)
    port map(
        clk     => clock_in,
        addr    => address(11 downto 2),
        cs_n    => boot_enable_n,
        we_n    => '1',
        data_i  => (others => '0'),
        data_o  => data_read_boot(23 downto 16)
    );

    boot1ub: entity work.boot_ram
    generic map (   memory_file => "boot.txt",
                    data_width => 8,
                    address_width => 12,
                    bank => 3)
    port map(
        clk     => clock_in,
        addr    => address(11 downto 2),
        cs_n    => boot_enable_n,
        we_n    => '1',
        data_i  => (others => '0'),
        data_o  => data_read_boot(31 downto 24)
    );

    -- RAM
    memory0lb: entity work.bram
    generic map (   memory_file => memory_file,
                    data_width => 8,
                    address_width => address_width,
                    bank => 0)
    port map(
        clk     => clock_in,
        addr    => address(address_width -1 downto 2),
        cs_n    => ram_enable_n,
        we_n    => data_w_n_ram(0),
        data_i  => data_write(7 downto 0),
        data_o  => data_read_ram(7 downto 0)
    );

    memory0ub: entity work.bram
    generic map (   memory_file => memory_file,
                    data_width => 8,
                    address_width => address_width,
                    bank => 1)
    port map(
        clk     => clock_in,
        addr    => address(address_width -1 downto 2),
        cs_n    => ram_enable_n,
        we_n    => data_w_n_ram(1),
        data_i  => data_write(15 downto 8),
        data_o  => data_read_ram(15 downto 8)
    );

    memory1lb: entity work.bram
    generic map (   memory_file => memory_file,
                    data_width => 8,
                    address_width => address_width,
                    bank => 2)
    port map(
        clk     => clock_in,
        addr    => address(address_width -1 downto 2),
        cs_n    => ram_enable_n,
        we_n    => data_w_n_ram(2),
        data_i  => data_write(23 downto 16),
        data_o  => data_read_ram(23 downto 16)
    );

    memory1ub: entity work.bram
    generic map (   memory_file => memory_file,
                    data_width => 8,
                    address_width => address_width,
                    bank => 3)
    port map(
        clk     => clock_in,
        addr    => address(address_width -1 downto 2),
        cs_n    => ram_enable_n,
        we_n    => data_w_n_ram(3),
        data_i  => data_write(31 downto 24),
        data_o  => data_read_ram(31 downto 24)
    );

    -- debug process
    debug:
    if uart_support = "no" generate
        process(clock_in, address)
            file store_file : text open write_mode is "debug.txt";
            variable hex_file_line : line;
            variable c : character;
            variable index : natural;
            variable line_length : natural := 0;
        begin
            if clock_in'event and clock_in = '1' then
                if address = x"f00000d0" and data = '0' then
                    data <= '1';
                    index := conv_integer(data_write(30 downto 24));
                    if index /= 10 then
                        c := character'val(index);
                        write(hex_file_line, c);
                        line_length := line_length + 1;
                    end if;
                    if index = 10 or line_length >= 72 then
                        writeline(store_file, hex_file_line);
                        line_length := 0;
                    end if;
                else
                    data <= '0';
                end if;
            end if;
        end process;
    end generate;

    process(clock_in, reset, address)
    begin
        if reset = '1' then
        elsif clock_in'event and clock_in = '0' then
            assert address /= x"e0000000" report "end of simulation" severity failure;
            assert (address < x"50000000") or (address >= x"e0000000") report "out of memory region" severity failure;
            assert address /= x"40000104" report "handling IRQ" severity warning;
        end if;
    end process;

end tb;